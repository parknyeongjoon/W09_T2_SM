#include "FLoaderFBX.h"
#include "Math/Quat.h"
#include "UserInterface/Console.h"

bool FLoaderFBX::LoadSkeletalMesh(const FString& FilePath, FSkeletalMeshData& OutData)
{
    OutData.Name = FilePath;
    std::filesystem::path FbxPath = FilePath;
    std::string           FbxUtf8    = FbxPath.string();

    // 1) Manager/IOSettings/Importer 세팅
    FbxManager* Manager = FbxManager::Create();
    FbxIOSettings* IOS = FbxIOSettings::Create(Manager, IOSROOT);
    Manager->SetIOSettings(IOS);
    FbxImporter* Importer = FbxImporter::Create(Manager, "");

    if (!Importer->Initialize(FbxUtf8.c_str(), -1, Manager->GetIOSettings())) {
        UE_LOG(LogLevel::Error, TEXT("FBX Import 실패: %s"), *FilePath);
        Importer->Destroy();
        Manager->Destroy();
        return false;
    }

    // 2) Scene 생성 및 Import
    FbxScene* Scene = FbxScene::Create(Manager, "Scene");
    Importer->Import(Scene);
    Importer->Destroy();

    // 씬 전체 삼각화
    FbxGeometryConverter GeometryConverter(Manager);
    if (!GeometryConverter.Triangulate(Scene, true, false))
    {
        UE_LOG(LogLevel::Error, TEXT("FBX Scene 삼각화에 실패했습니다."));
    }

    // 3) Skeleton 파싱
    TArray<FBoneInfo> Bones;
    ParseSkeletonRecursive(Scene->GetRootNode(), Bones, -1);
    ComputeGlobalBindPoses(Bones);
    OutData.Bones = Bones;

    // 4) Mesh 정보 추출
    TArray<FbxMesh*> Meshes;
    CollectMeshes(Scene->GetRootNode(), Meshes);

    TArray<FSkinnedVertex>    Vertices;
    TArray<FSkinWeight>        Weights;
    TArray<uint32>             Indices;
    TMap<int32, TArray<int32>> CP2Verts;  // control-point → 버텍스 인덱스 맵
    for (FbxMesh* Mesh : Meshes)
        ExtractMeshData(Mesh, Vertices, Indices, CP2Verts);

    FSkinWeight DefaultW;
    for (int k = 0; k < 4; ++k)
    {
        DefaultW.BoneIndices[k] = -1;
        DefaultW.BoneWeights[k] = 0.f;
    }
    Weights.Init(DefaultW, Vertices.Num());

    // 5) Skin Weight 적용
    for (FbxMesh* Mesh : Meshes)
        ParseSkinWeights(Mesh, Weights, Bones, CP2Verts);

    // 6) OutData에 메시 데이터 바인딩
    OutData.Vertices = std::move(Vertices);
    OutData.Indices  = std::move(Indices);
    OutData.Weights = std::move(Weights);

    // 7) 리소스 정리 및 반환
    Manager->Destroy();
    return true;
}

bool FLoaderFBX::LoadStaticMesh(const FString& FilePath, OBJ::FStaticMeshRenderData& OutData)
{
    return false;
}

void FLoaderFBX::ParseSkeletonRecursive(FbxNode* Node, TArray<FBoneInfo>& BoneList, int32 ParentIndex)
{
    if (!Node) return;
    
    // 이 노드가 'Skeleton' 속성인지 검사
    FbxNodeAttribute* Attr = Node->GetNodeAttribute();
    int32 MyIndex = ParentIndex;

    if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
        FBoneInfo Bone;
        Bone.Name        = FString(Node->GetName());
        Bone.ParentIndex = ParentIndex;

        // 0프레임의 로컬 공간 기준 변환 행렬 가져옴 (바인드 포즈, T-pose)
        FbxTime time = FBXSDK_TIME_ZERO;;
        FbxAMatrix fbxLocal = Node->EvaluateLocalTransform(time);

        // FBX 행렬 → 엔진 행렬 변환
        FMatrix localMat;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                localMat.M[i][j] = static_cast<float>(fbxLocal.Get(i, j));
        Bone.LocalTransform = localMat;
        
        BoneList.Add(Bone);
        MyIndex = BoneList.Num() - 1;
    }

    // 모든 자식 노드에 대해 재귀 호출
    const int childCount  = Node->GetChildCount();
    for (int32 i = 0; i < childCount; ++i) {
        ParseSkeletonRecursive(Node->GetChild(i), BoneList, MyIndex);
    }
}

void FLoaderFBX::ComputeGlobalBindPoses(TArray<FBoneInfo>& Bones)
{
    for (int32 i = 0; i < Bones.Num(); ++i)
    {
        Bones[i].GlobalReferencePose = Bones[i].ParentIndex >= 0 ?
                                            Bones[Bones[i].ParentIndex].GlobalReferencePose * Bones[i].LocalTransform :
                                            Bones[i].LocalTransform;  // Root Bone
        
        Bones[i].InverseBindPose = FMatrix::Inverse(Bones[i].GlobalReferencePose);
    }
}

void FLoaderFBX::CollectMeshes(FbxNode* Node, TArray<FbxMesh*>& OutMeshes)
{
    if (!Node) return;
    if (auto* Mesh = Node->GetMesh())
    {
        OutMeshes.Add(Mesh);
    }
    for (int i =0; i<Node->GetChildCount(); ++i)
    {
        CollectMeshes(Node->GetChild(i), OutMeshes);
    }
}

void FLoaderFBX::ExtractMeshData(FbxMesh* Mesh,
                     TArray<FSkinnedVertex>& OutVertices,
                     TArray<uint32>& OutIndices,
                     TMap<int32, TArray<int32>>& OutCP2Verts)
{
    auto* ControlPoints = Mesh->GetControlPoints();
    int PolygonCount = Mesh->GetPolygonCount();
    FbxGeometryElementNormal* NormalElem = Mesh->GetElementNormal();
    FbxGeometryElementUV*     UVElem     = Mesh->GetElementUV(0);

    int polyVertexIndex = 0;
    for (int PolyIndex = 0; PolyIndex < PolygonCount; ++PolyIndex)
    {
        int VertCount = Mesh->GetPolygonSize(PolyIndex);
        for (int VertIdx = 0; VertIdx < VertCount; ++VertIdx)
        {
            int CPIndex = Mesh->GetPolygonVertex(PolyIndex, VertIdx);
            FbxVector4 Pos = ControlPoints[CPIndex];

            // Vertex 생성
            FSkinnedVertex Vtx;
            Vtx.Position = FVector((float)Pos[0], (float)Pos[1], (float)Pos[2]);

            // Normal 추출
            if (NormalElem)
            {
                FbxVector4 Norm;
                if (NormalElem->GetMappingMode() == FbxGeometryElement::eByControlPoint)
                {
                    Norm = NormalElem->GetDirectArray().GetAt(CPIndex);
                }
                else if (NormalElem->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
                {
                    int normalIndex = (NormalElem->GetReferenceMode() == FbxGeometryElement::eDirect)
                        ? polyVertexIndex
                        : NormalElem->GetIndexArray().GetAt(polyVertexIndex);
                    Norm = NormalElem->GetDirectArray().GetAt(normalIndex);
                }
                Vtx.Normal = FVector((float)Norm[0], (float)Norm[1], (float)Norm[2]);
            }

            // UV 추출
            if (UVElem)
            {
                FbxVector2 UV;
                if (UVElem->GetMappingMode() == FbxGeometryElement::eByControlPoint)
                {
                    UV = UVElem->GetDirectArray().GetAt(CPIndex);
                }
                else if (UVElem->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
                {
                    int uvIndex = UVElem->GetIndexArray().GetAt(polyVertexIndex);
                    UV = UVElem->GetDirectArray().GetAt(uvIndex);
                }
                Vtx.UV = FVector2D((float)UV[0], (float)UV[1]);
            }

            int NewIndex = OutVertices.Add(Vtx);
            OutIndices.Add(NewIndex);
            OutCP2Verts.FindOrAdd(CPIndex).Add(NewIndex);
        }
    }
}

void FLoaderFBX::ParseSkinWeights(
    FbxMesh*                            Mesh,
    TArray<FSkinWeight>&                Weights,
    const TArray<FBoneInfo>&            Bones,
    const TMap<int32, TArray<int32>>&   CP2Verts)
{
    FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));
    if (!Skin) return;

    // 클러스터(본) 순회
    for (int c = 0; c < Skin->GetClusterCount(); ++c)
    {
        FbxCluster* Cluster = Skin->GetCluster(c);
        FString BoneName( Cluster->GetLink()->GetName() );

        int BoneIndex = -1;
        for (int i=0; i<Bones.Num(); ++i)
        {
            if (Bones[i].Name == BoneName)
            {
                BoneIndex = i;
                break;
            }
        }
        
        if (BoneIndex == INDEX_NONE) continue;
    
        // 각 컨트롤 포인트 영향
        int Count = Cluster->GetControlPointIndicesCount();
        int* CtrlPoints = Cluster->GetControlPointIndices();
        double* CtrlWeights     = Cluster->GetControlPointWeights();
        for (int i = 0; i < Count; ++i) {
            int CPIdx = CtrlPoints[i];
            float  W    = (float)CtrlWeights[i];
            
            // 해당 제어점이 참조한 버텍스들만 순회
            const TArray<int32>* VtxList = CP2Verts.Find(CPIdx);
            if (!VtxList) continue;
            
            for (int VtxIdx : *VtxList)
            {
                FSkinWeight& SW = Weights[VtxIdx];
                for (int k = 0; k < 4; ++k)
                {
                    if (SW.BoneWeights[k] == 0.f)
                    {
                        SW.BoneIndices[k] = BoneIndex;
                        SW.BoneWeights[k] = W;
                        break;
                    }
                }
            }
        }
    }

    // 웨이트 정규화
    for (FSkinWeight& SW : Weights) {
        float WeightSum = SW.BoneWeights[0]
                    + SW.BoneWeights[1]
                    + SW.BoneWeights[2]
                    + SW.BoneWeights[3];
        if (WeightSum > 0.f)
            for (int k = 0; k < 4; ++k)
                SW.BoneWeights[k] /= WeightSum;
    }
}

void FLoaderFBX::ParseStaticMesh(FbxMesh* Mesh, OBJ::FStaticMeshRenderData& OutData)
{
}