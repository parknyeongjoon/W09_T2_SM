#include "FLoaderFBX.h"
#include "Math/Quat.h"
#include "UserInterface/Console.h"

bool FLoaderFBX::LoadSkeletalMesh(const FString& FilePath, FSkeletalMeshData& OutData)
{
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

    // 3) 스켈레톤 파싱
    TArray<FBoneInfo> Bones;
    ParseSkeletonRecursive(Scene->GetRootNode(), Bones, -1);
    ComputeGlobalBindPoses(Bones);
    OutData.Bones = Bones;

    // 4) Mesh 정보 추출
    TArray<FbxMesh*> Meshes;
    CollectMeshes(Scene->GetRootNode(), Meshes);

    TArray<FSkinnedVertex>    Vertices;
    TArray<uint32>             Indices;
    TMap<int32, TArray<int32>> CP2Verts;  // control-point → 버텍스 인덱스 맵

    // 5) Skin Weight 적용
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

void ExtractMeshData(FbxMesh* Mesh, TArray<FSkinnedVertex>& OutVertices, TArray<uint32>& OutIndices, TMap<int32, TArray<int32>>& OutCP2Verts)
{
    auto* ControlPoints = Mesh->GetControlPoints();
    int PolygonCount = Mesh->GetPolygonCount();

    for (int PolyIndex = 0; PolyIndex < PolygonCount; ++PolyIndex)
    {
        int VertCount = Mesh->GetPolygonSize(PolyIndex);
        for (int VertIdx = 0; VertIdx < VertCount; ++VertIdx)
        {
            int CPIndex = Mesh->GetPolygonVertex(PolyIndex, VertIdx);
            FbxVector4 Pos = ControlPoints[CPIndex];

            // 버텍스 생성
            FSkinnedVertex Vtx;
            Vtx.Position = FVector((float)Pos[0], (float)Pos[1], (float)Pos[2]);
            // TODO: Normals, UV 등 추가 정보 설정

            int NewIndex = OutVertices.Add(Vtx);
            OutIndices.Add(NewIndex);

            OutCP2Verts.FindOrAdd(CPIndex).Add(NewIndex);
        }
    }
}

void FLoaderFBX::ParseSkinWeights(FbxMesh* Mesh, TArray<FSkinnedVertex>& Vertices, const TArray<FBoneInfo>& Bones)
{
    FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));
    if (!Skin) return;

    // 클러스터(본) 순회
    // for (int32 c = 0; c < Skin->GetClusterCount(); ++c) {
    //     FbxCluster* Cluster = Skin->GetCluster(c);
    //     FString BoneName = UTF8_TO_TCHAR(Cluster->GetLink()->GetName());
    //     int32 BoneIndex = Bones.IndexOfByPredicate([&](auto& B){ return B.Name == BoneName; });
    //     if (BoneIndex == INDEX_NONE) continue;
    //
    //     // 각 컨트롤 포인트 영향
    //     int Count = Cluster->GetControlPointIndicesCount();
    //     auto CtrlPoints = Cluster->GetControlPointIndices();
    //     auto Weights     = Cluster->GetControlPointWeights();
    //     for (int32 i = 0; i < Count; ++i) {
    //         int32 CPIdx = CtrlPoints[i];
    //         float  W    = (float)Weights[i];
    //
    //         // 해당 CP에 매핑된 모든 버텍스에 적용
    //         // (위에서 CP→Verts 맵을 만들어 두면 좋습니다)
    //         // 예시는 모든 Vert에 뿌려버림(프로젝트에 맞춰 최적화 필요)
    //         for (FSkeletalVertex& V : Vertices) {
    //             // 단순 예시: 첫번째 빈 슬롯에 채우기
    //             for (int k = 0; k < 4; ++k) {
    //                 if (V.BoneWeights[k] == 0.f) {
    //                     V.BoneIndices[k] = BoneIndex;
    //                     V.BoneWeights[k] = W;
    //                     break;
    //                 }
    //             }
    //         }
    //     }
    // }

    // 웨이트 정규화
    for (FSkinnedVertex& V : Vertices) {
        float Sum = V.BoneWeights[0] + V.BoneWeights[1] + V.BoneWeights[2] + V.BoneWeights[3];
        if (Sum > 0) {
            for (int k = 0; k < 4; ++k)
                V.BoneWeights[k] /= Sum;
        }
    }
}

void FLoaderFBX::ParseStaticMesh(FbxMesh* Mesh, OBJ::FStaticMeshRenderData& OutData)
{
}