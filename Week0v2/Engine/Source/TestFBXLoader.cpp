#include "TestFBXLoader.h"
#include "Components/Material/Material.h"
#include "Engine/FLoaderOBJ.h"

bool TestFBXLoader::InitFBXManager()
{
    if (bInitialized)
        return true;

    FbxManager = FbxManager::Create();

    FbxIOSettings* IOSetting = FbxIOSettings::Create(FbxManager, IOSROOT);
    FbxManager->SetIOSettings(IOSetting);

    bInitialized = true;

    return true;
}

bool TestFBXLoader::InitFBX(const FString& FilePath)
{
    FbxImporter* Importer = FbxImporter::Create(FbxManager, "myImporter");
    FbxScene* Scene = FbxScene::Create(FbxManager, "myScene");
    
    bool bResult = Importer->Initialize(GetData("Contents\\" + FilePath), -1, FbxManager->GetIOSettings());
    if (!bResult)
        return false;

    Importer->Import(Scene);
    
    Importer->Destroy();

    FSkeletalMeshRenderData* NewMeshData = new FSkeletalMeshRenderData();
    FRefSkeletal* RefSkeletal = new FRefSkeletal();
    
    NewMeshData->Name = FilePath;
    RefSkeletal->Name = FilePath;
    
    ExtractFBXMeshData(Scene, NewMeshData, RefSkeletal);
    
    SkeletalMeshData.Add(FilePath, NewMeshData);
    RefSkeletalData.Add(FilePath, RefSkeletal);

    Scene->Destroy();
    
    return true;
}

void TestFBXLoader::ExtractFBXMeshData(const FbxScene* Scene, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal)
{
    FbxNode* RootNode = Scene->GetRootNode();
    if (RootNode == nullptr)
        return;

    ExtractMeshFromNode(RootNode, MeshData, RefSkeletal);
}

void TestFBXLoader::ExtractMeshFromNode(FbxNode* Node, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal)
{
    FbxMesh* Mesh = Node->GetMesh();
    // Mesh가 있다면 데이터 저장
    if (Mesh)
    {
        // 버텍스 데이터 추출
        ExtractVertices(Mesh, MeshData, RefSkeletal);
        
        // 인덱스 데이터 추출
        ExtractIndices(Mesh, MeshData);
        
        // 머테리얼 데이터 추출
        ExtractMaterials(Node, Mesh, MeshData, RefSkeletal);
        
        // 바운딩 박스 업데이트
        UpdateBoundingBox(*MeshData);
    }

    // 자식 노드들에 대해 재귀적으로 수행
    int childCount = Node->GetChildCount();
    for (int i = 0; i < childCount; i++) {
        ExtractMeshFromNode(Node->GetChild(i), MeshData, RefSkeletal);
    }
}

void TestFBXLoader::ExtractVertices(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal)
{
    int VertexCount = Mesh->GetControlPointsCount();
    FbxVector4* ControlPoints = Mesh->GetControlPoints();
    
    // 기존 정점 개수를 저장 (여러 메쉬가 있을 경우 인덱스 오프셋으로 사용)
    int BaseVertexIndex = MeshData->Vertices.Num();
    
    // 정점 위치 데이터 추출
    for (int i = 0; i < VertexCount; i++)
    {
        FSkeletalVertex Vertex;
        
        // 위치 설정
        Vertex.Position.X = static_cast<float>(ControlPoints[i][0]);
        Vertex.Position.Y = static_cast<float>(ControlPoints[i][1]);
        Vertex.Position.Z = static_cast<float>(ControlPoints[i][2]);
        Vertex.Position.W = 1;
        
        // 기본값으로 초기화
        Vertex.Normal = FVector(0.0f, 0.0f, 1.0f);
        Vertex.TexCoord = FVector2D(0.0f, 0.0f);
        Vertex.Tangent = FVector(1.0f, 0.0f, 0.0f);
        
        MeshData->Vertices.Add(Vertex);
    }
    
    // 법선 데이터 추출
    ExtractNormals(Mesh, MeshData, BaseVertexIndex);
    
    // UV 데이터 추출
    ExtractUVs(Mesh, MeshData, BaseVertexIndex);
    
    // 탄젠트 데이터 추출
    ExtractTangents(Mesh, MeshData, BaseVertexIndex);

    // 스키닝 정보 추출 (bone weight 추출)
    ExtractSkinningData(Mesh, MeshData, RefSkeletal, BaseVertexIndex);

    for (int i=0;i<VertexCount;i++)
    {
        FSkeletalVertex Vertex;
        Vertex = MeshData->Vertices[i];
        RefSkeletal->RawVertices.Add(Vertex);
    }
}

void TestFBXLoader::ExtractNormals(FbxMesh* Mesh, FSkeletalMeshRenderData* RenderData, int BaseVertexIndex)
{
    FbxGeometryElementNormal* NormalElement = Mesh->GetElementNormal();
    if (!NormalElement)
        return;
        
    FbxGeometryElement::EMappingMode MappingMode = NormalElement->GetMappingMode();
    FbxGeometryElement::EReferenceMode ReferenceMode = NormalElement->GetReferenceMode();
    
    int PolygonCount = Mesh->GetPolygonCount();
    
    // 폴리곤 별로 순회하며 법선 데이터 추출
    for (int PolyIndex = 0; PolyIndex < PolygonCount; PolyIndex++)
    {
        int PolySize = Mesh->GetPolygonSize(PolyIndex);
        
        for (int VertexIndex = 0; VertexIndex < PolySize; VertexIndex++)
        {
            int ControlPointIndex = Mesh->GetPolygonVertex(PolyIndex, VertexIndex);
            
            // 매핑 모드에 따른 법선 인덱스 계산
            int NormalIndex = 0;
            
            if (MappingMode == FbxGeometryElement::eByControlPoint)
            {
                if (ReferenceMode == FbxGeometryElement::eDirect)
                    NormalIndex = ControlPointIndex;
                else if (ReferenceMode == FbxGeometryElement::eIndexToDirect)
                    NormalIndex = NormalElement->GetIndexArray().GetAt(ControlPointIndex);
            }
            else if (MappingMode == FbxGeometryElement::eByPolygonVertex)
            {
                int VertexId = PolyIndex * 3 + VertexIndex;
                
                if (ReferenceMode == FbxGeometryElement::eDirect)
                    NormalIndex = VertexId;
                else if (ReferenceMode == FbxGeometryElement::eIndexToDirect)
                    NormalIndex = NormalElement->GetIndexArray().GetAt(VertexId);
            }
            
            // 법선 데이터 가져오기
            FbxVector4 Normal = NormalElement->GetDirectArray().GetAt(NormalIndex);
            
            // 해당 정점의 법선 설정
            RenderData->Vertices[BaseVertexIndex + ControlPointIndex].Normal.X = static_cast<float>(Normal[0]);
            RenderData->Vertices[BaseVertexIndex + ControlPointIndex].Normal.Y = static_cast<float>(Normal[1]);
            RenderData->Vertices[BaseVertexIndex + ControlPointIndex].Normal.Z = static_cast<float>(Normal[2]);
        }
    }
}

void TestFBXLoader::ExtractUVs(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, int BaseVertexIndex)
{
    FbxGeometryElementUV* UVElement = Mesh->GetElementUV(0); // 첫 번째 UV 채널 사용
    if (!UVElement)
        return;
        
    FbxGeometryElement::EMappingMode MappingMode = UVElement->GetMappingMode();
    FbxGeometryElement::EReferenceMode ReferenceMode = UVElement->GetReferenceMode();
    
    int PolygonCount = Mesh->GetPolygonCount();
    
    for (int PolyIndex = 0; PolyIndex < PolygonCount; PolyIndex++)
    {
        int PolySize = Mesh->GetPolygonSize(PolyIndex);
        
        for (int VertexIndex = 0; VertexIndex < PolySize; VertexIndex++)
        {
            int ControlPointIndex = Mesh->GetPolygonVertex(PolyIndex, VertexIndex);
            int UVIndex = Mesh->GetTextureUVIndex(PolyIndex, VertexIndex);
            
            // 매핑 모드에 따른 UV 인덱스 계산
            if (MappingMode == FbxGeometryElement::eByControlPoint)
            {
                if (ReferenceMode == FbxGeometryElement::eDirect)
                    UVIndex = ControlPointIndex;
                else if (ReferenceMode == FbxGeometryElement::eIndexToDirect)
                    UVIndex = UVElement->GetIndexArray().GetAt(ControlPointIndex);
            }
            else if (MappingMode == FbxGeometryElement::eByPolygonVertex)
            {
                if (ReferenceMode == FbxGeometryElement::eDirect || ReferenceMode == FbxGeometryElement::eIndexToDirect)
                {
                    // UVIndex는 이미 Mesh->GetTextureUVIndex에서 계산됨
                }
            }
            
            // UV 데이터 가져오기
            FbxVector2 UV = UVElement->GetDirectArray().GetAt(UVIndex);
            
            // 해당 정점의 UV 설정
            MeshData->Vertices[BaseVertexIndex + ControlPointIndex].TexCoord.X = static_cast<float>(UV[0]);
            MeshData->Vertices[BaseVertexIndex + ControlPointIndex].TexCoord.Y = 1.0f - static_cast<float>(UV[1]); // DirectX UV 좌표계로 변환
        }
    }
}

void TestFBXLoader::ExtractTangents(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, int BaseVertexIndex)
{
    FbxGeometryElementTangent* TangentElement = Mesh->GetElementTangent();
    if (!TangentElement)
        return;
        
    FbxGeometryElement::EMappingMode MappingMode = TangentElement->GetMappingMode();
    FbxGeometryElement::EReferenceMode ReferenceMode = TangentElement->GetReferenceMode();
    
    int PolygonCount = Mesh->GetPolygonCount();
    
    for (int PolyIndex = 0; PolyIndex < PolygonCount; PolyIndex++)
    {
        int PolySize = Mesh->GetPolygonSize(PolyIndex);
        
        for (int VertexIndex = 0; VertexIndex < PolySize; VertexIndex++)
        {
            int ControlPointIndex = Mesh->GetPolygonVertex(PolyIndex, VertexIndex);
            
            // 매핑 모드에 따른 탄젠트 인덱스 계산
            int TangentIndex = 0;
            
            if (MappingMode == FbxGeometryElement::eByControlPoint)
            {
                if (ReferenceMode == FbxGeometryElement::eDirect)
                    TangentIndex = ControlPointIndex;
                else if (ReferenceMode == FbxGeometryElement::eIndexToDirect)
                    TangentIndex = TangentElement->GetIndexArray().GetAt(ControlPointIndex);
            }
            else if (MappingMode == FbxGeometryElement::eByPolygonVertex)
            {
                int VertexId = PolyIndex * 3 + VertexIndex;
                
                if (ReferenceMode == FbxGeometryElement::eDirect)
                    TangentIndex = VertexId;
                else if (ReferenceMode == FbxGeometryElement::eIndexToDirect)
                    TangentIndex = TangentElement->GetIndexArray().GetAt(VertexId);
            }
            
            // 탄젠트 데이터 가져오기
            FbxVector4 Tangent = TangentElement->GetDirectArray().GetAt(TangentIndex);
            
            // 해당 정점의 탄젠트 설정
            MeshData->Vertices[BaseVertexIndex + ControlPointIndex].Tangent.X = static_cast<float>(Tangent[0]);
            MeshData->Vertices[BaseVertexIndex + ControlPointIndex].Tangent.Y = static_cast<float>(Tangent[1]);
            MeshData->Vertices[BaseVertexIndex + ControlPointIndex].Tangent.Z = static_cast<float>(Tangent[2]);
        }
    }
}

void TestFBXLoader::ExtractSkinningData(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal, int BaseVertexIndex)
{
    int VertexCount = Mesh->GetControlPointsCount();
    for (int i=BaseVertexIndex; i<BaseVertexIndex+VertexCount; i++)
    {
        // Reset bone indices and weights
        for (int j = 0; j < 4; j++)
        {
            MeshData->Vertices[i].BoneIndices[j] = 0;
            MeshData->Vertices[i].BoneWeights[j] = 0.0f;
        }
    }

    // Get deformer count (skins)
    int DeformerCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
    if (DeformerCount == 0)
        return;
        
    // Process each skin deformer
    for (int DeformerIndex = 0; DeformerIndex < DeformerCount; DeformerIndex++)
    {
        FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(DeformerIndex, FbxDeformer::eSkin));
        if (Skin)
        {
            ProcessSkinning(Skin, MeshData, RefSkeletal, BaseVertexIndex);
        }
    }

    // Normalize bone weights
    for (int i = BaseVertexIndex; i < MeshData->Vertices.Num(); i++)
    {
        float Sum = 0.0f;
        for (int j = 0; j < 4; j++)
        {
            Sum += MeshData->Vertices[i].BoneWeights[j];
        }
        
        if (Sum > 0.0f)
        {
            for (int j = 0; j < 4; j++)
            {
                MeshData->Vertices[i].BoneWeights[j] /= Sum;
            }
        }
        else
        {
            // If no weights, bind to first bone
            MeshData->Vertices[i].BoneIndices[0] = 0;
            MeshData->Vertices[i].BoneWeights[0] = 1.0f;
        }
    }
}

void TestFBXLoader::ProcessSkinning(FbxSkin* Skin, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal, int BaseVertexIndex)
{
    int ClusterCount = Skin->GetClusterCount();

    // Clear existing bone tree data
    RefSkeletal->BoneTree.Empty();
    RefSkeletal->RootBoneIndices.Empty();
    RefSkeletal->BoneNameToIndexMap.Empty();
    
    // First pass - collect all bone nodes from clusters and add to flat bone array
    for (int ClusterIndex = 0; ClusterIndex < ClusterCount; ClusterIndex++)
    {
        FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
        FbxNode* BoneNode = Cluster->GetLink();
        
        if (!BoneNode)
            continue;
            
        FString BoneName = BoneNode->GetName();
        
        // Check if this bone already exists
        int* ExistingBoneIndex = RefSkeletal->BoneNameToIndexMap.Find(BoneName);
        if (ExistingBoneIndex)
            continue;
            
        // Create new bone and add to array
        FBone NewBone;
        NewBone.BoneName = BoneName;
        NewBone.ParentIndex = -1; // Will be set in second pass
        
        // Get transform matrices
        FbxAMatrix GlobalTransform = BoneNode->EvaluateGlobalTransform();
        FbxAMatrix LocalTransform = BoneNode->EvaluateLocalTransform();
        
        // Store transforms
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                NewBone.GlobalTransform.M[i][j] = static_cast<float>(GlobalTransform.Get(i, j));
                NewBone.LocalTransform.M[i][j] = static_cast<float>(LocalTransform.Get(i, j));
            }
        }
        
        // Get binding pose transformation
        //  NewBone.InverseBindPoseMatrix = FMatrix::Inverse(NewBone.GlobalTransform);
        FbxAMatrix MeshTransform;
        Cluster->GetTransformMatrix(MeshTransform); // 메시의 변환 행렬 (기준점)

        FbxAMatrix LinkTransform;
        Cluster->GetTransformLinkMatrix(LinkTransform); // 본의 바인드 포즈 행렬
        
        FbxAMatrix InverseBindMatrix = LinkTransform.Inverse() * MeshTransform;

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                NewBone.InverseBindPoseMatrix.M[i][j] = static_cast<float>(InverseBindMatrix.Get(i, j));
            }
        }
        
        NewBone.SkinningMatrix = NewBone.InverseBindPoseMatrix * NewBone.GlobalTransform;
        
        // Add bone to array and create mapping
        int BoneIndex = MeshData->Bones.Add(NewBone);
        RefSkeletal->BoneNameToIndexMap.Add(BoneName, BoneIndex);
        
        // Create corresponding bone tree node
        FBoneNode NewNode;
        NewNode.BoneName = BoneName;
        NewNode.BoneIndex = BoneIndex;
        RefSkeletal->BoneTree.Add(NewNode);
    }
    
    // Second pass - establish parent-child relationships
    for (int ClusterIndex = 0; ClusterIndex < ClusterCount; ClusterIndex++)
    {
        FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
        FbxNode* BoneNode = Cluster->GetLink();
        
        if (!BoneNode)
            continue;
            
        FString BoneName = BoneNode->GetName();
        FbxNode* ParentNode = BoneNode->GetParent();
        
        if (!RefSkeletal->BoneNameToIndexMap.Contains(BoneName))
            continue;
            
        int BoneIndex = RefSkeletal->BoneNameToIndexMap[BoneName];
        
        if (ParentNode)
        {
            FString ParentName = ParentNode->GetName();
            
            // If parent is also a bone, establish the relationship
            if (RefSkeletal->BoneNameToIndexMap.Contains(ParentName))
            {
                int ParentIndex = RefSkeletal->BoneNameToIndexMap[ParentName];
                
                // Update parent index in the bone
                MeshData->Bones[BoneIndex].ParentIndex = ParentIndex;
                
                // Add this bone as a child of the parent in the tree structure
                RefSkeletal->BoneTree[ParentIndex].ChildIndices.Add(BoneIndex);
            }
        }
    }
    
    // Find root bones (bones with no parents in our bone set)
    for (int i = 0; i < MeshData->Bones.Num(); i++)
    {
        if (MeshData->Bones[i].ParentIndex == -1)
        {
            RefSkeletal->RootBoneIndices.Add(i);
        }
    }
    
    // Process vertex weights
    for (int ClusterIndex = 0; ClusterIndex < ClusterCount; ClusterIndex++)
    {
        FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
        FbxNode* BoneNode = Cluster->GetLink();
        
        if (!BoneNode || !RefSkeletal->BoneNameToIndexMap.Contains(BoneNode->GetName()))
            continue;
        
        int BoneIndex = RefSkeletal->BoneNameToIndexMap[BoneNode->GetName()];
        
        // Get control point indices and weights
        int VertexCount = Cluster->GetControlPointIndicesCount();
        int* ControlPointIndices = Cluster->GetControlPointIndices();
        double* ControlPointWeights = Cluster->GetControlPointWeights();
        
        // Apply weights to vertices
        for (int i = 0; i < VertexCount; i++)
        {
            int VertexIndex = BaseVertexIndex + ControlPointIndices[i];
            float Weight = static_cast<float>(ControlPointWeights[i]);
            
            // Make sure vertex index is valid
            if (VertexIndex >= 0 && VertexIndex < MeshData->Vertices.Num())
            {
                // Find first empty weight slot
                for (int j = 0; j < 4; j++)
                {
                    if (MeshData->Vertices[VertexIndex].BoneWeights[j] == 0.0f)
                    {
                        MeshData->Vertices[VertexIndex].BoneIndices[j] = BoneIndex;
                        MeshData->Vertices[VertexIndex].BoneWeights[j] = Weight;
                        break;
                    }
                }
            }
        }
    }
}

void TestFBXLoader::ExtractIndices(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData)
{
    int PolygonCount = Mesh->GetPolygonCount();
    int BaseVertexIndex = MeshData->Vertices.Num() - Mesh->GetControlPointsCount();
    
    // 폴리곤 별로 순회하며 인덱스 데이터 추출
    for (int PolyIndex = 0; PolyIndex < PolygonCount; PolyIndex++)
    {
        int PolySize = Mesh->GetPolygonSize(PolyIndex);
        
        // 삼각형으로 분할 (폴리곤이 4각형 이상일 경우)
        for (int i = 2; i < PolySize; i++)
        {
            // 삼각형 구성 (0, i-1, i)
            int ControlPoint0 = Mesh->GetPolygonVertex(PolyIndex, 0);
            int ControlPoint1 = Mesh->GetPolygonVertex(PolyIndex, i - 1);
            int ControlPoint2 = Mesh->GetPolygonVertex(PolyIndex, i);
            
            // 정점 인덱스 추가 (BaseVertexIndex 오프셋 적용)
            MeshData->Indices.Add(BaseVertexIndex + ControlPoint0);
            MeshData->Indices.Add(BaseVertexIndex + ControlPoint1);
            MeshData->Indices.Add(BaseVertexIndex + ControlPoint2);
        }
    }
}

void TestFBXLoader::ExtractMaterials(FbxNode* Node, FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal)
{
    int MaterialCount = Node->GetMaterialCount();
    int BaseIndexOffset = MeshData->Indices.Num();
    int TotalTriangleCount = 0;
    for (int PolyIndex = 0; PolyIndex < Mesh->GetPolygonCount(); PolyIndex++) {
        int PolySize = Mesh->GetPolygonSize(PolyIndex);
        TotalTriangleCount += (PolySize - 2); // 폴리곤을 삼각형으로 분할한 개수
    }
    BaseIndexOffset -= TotalTriangleCount * 3; // 총 삼각형 개수 * 3
    
    // 재질 정보 추출
    for (int i = 0; i < MaterialCount; i++)
    {
        FbxSurfaceMaterial* FbxMaterial = Node->GetMaterial(i);
        if (!FbxMaterial)
            continue;
            
        // 재질 이름 가져오기
        FString MaterialName = FbxMaterial->GetName();
        
        // 엔진 재질 생성 또는 검색
        UMaterial* Material = FManagerOBJ::GetMaterial(MaterialName);
        
        // 재질 추가
        int MaterialIndex = RefSkeletal->Materials.Add(Material);
        
        // 폴리곤을 순회하며 현재 재질에 해당하는 폴리곤 수 계산
        int PolygonCount = Mesh->GetPolygonCount();
        int TriangleCount = 0;
        
        for (int PolyIndex = 0; PolyIndex < PolygonCount; PolyIndex++)
        {
            int MaterialIndexForPolygon = Mesh->GetElementMaterial()->GetIndexArray().GetAt(PolyIndex);
            
            if (MaterialIndexForPolygon == i)
            {
                int PolySize = Mesh->GetPolygonSize(PolyIndex);
                TriangleCount += (PolySize - 2); // 삼각형 개수 계산
            }
        }

        // 재질 서브셋 정보 구성
        FMaterialSubset Subset;
        Subset.MaterialIndex = MaterialIndex;
        Subset.IndexStart = BaseIndexOffset; // 시작 인덱스
        Subset.MaterialName = MaterialName;
        Subset.IndexCount = TriangleCount * 3; // 삼각형 당 인덱스 3개
        RefSkeletal->MaterialSubsets.Add(Subset);

        BaseIndexOffset += TriangleCount * 3;
    }
    
    // 메시에 재질이 없는 경우 기본 재질 생성
    if (MaterialCount == 0)
    {
        UMaterial* DefaultMaterial = FManagerOBJ::GetDefaultMaterial();
        int MaterialIndex = RefSkeletal->Materials.Add(DefaultMaterial);
        
        FMaterialSubset Subset;
        Subset.MaterialName = DefaultMaterial->GetName();
        Subset.MaterialIndex = MaterialIndex;
        Subset.IndexStart = BaseIndexOffset;
        Subset.IndexCount = MeshData->Indices.Num() - BaseIndexOffset;
        
        RefSkeletal->MaterialSubsets.Add(Subset);
    }
}

void TestFBXLoader::UpdateBoundingBox(FSkeletalMeshRenderData MeshData)
{
    if (MeshData.Vertices.Num() == 0)
        return;
        
    // 초기값 설정
    FVector Min = MeshData.Vertices[0].Position.xyz();
    FVector Max = MeshData.Vertices[0].Position.xyz();
    
    // 모든 정점을 순회하며 최소/최대값 업데이트
    for (int i = 1; i < MeshData.Vertices.Num(); i++)
    {
        const FVector& Pos = MeshData.Vertices[i].Position.xyz();
        
        // 최소값 갱신
        Min.X = FMath::Min(Min.X, Pos.X);
        Min.Y = FMath::Min(Min.Y, Pos.Y);
        Min.Z = FMath::Min(Min.Z, Pos.Z);
        
        // 최대값 갱신
        Max.X = FMath::Max(Max.X, Pos.X);
        Max.Y = FMath::Max(Max.Y, Pos.Y);
        Max.Z = FMath::Max(Max.Z, Pos.Z);
    }
    
    // 바운딩 박스 설정
    MeshData.BoundingBox.min = Min;
    MeshData.BoundingBox.max = Max;
}

FSkeletalMeshRenderData* TestFBXLoader::GetSkeletalRenderData(FString FilePath)
{
    // TODO: 폴더에서 가져올 수 있으면 가져오기
    if (SkeletalMeshData.Contains(FilePath))
    {
        return SkeletalMeshData[FilePath];
    }
    
    return nullptr;
}

FSkeletalMeshRenderData TestFBXLoader::GetCopiedSkeletalRenderData(FString FilePath)
{
    // 있으면 가져오고
    FSkeletalMeshRenderData* OriginRenderData = SkeletalMeshData[FilePath];
    if (OriginRenderData != nullptr)
    {
        return *OriginRenderData;
    }
    // 없으면 기본 생성자
    return {};
}

FRefSkeletal* TestFBXLoader::GetRefSkeletal(FString FilePath)
{
    // TODO: 폴더에서 가져올 수 있으면 가져오기
    if (RefSkeletalData.Contains(FilePath))
    {
        return RefSkeletalData[FilePath];
    }
    
    return nullptr;
}
