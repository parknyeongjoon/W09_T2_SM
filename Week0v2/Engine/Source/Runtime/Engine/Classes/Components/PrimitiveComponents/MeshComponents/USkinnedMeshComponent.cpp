#include "USkinnedMeshComponent.h"
#include "LaunchEngineLoop.h"
#include "Renderer/Renderer.h"

USkinnedMeshComponent::USkinnedMeshComponent(const USkinnedMeshComponent& Other)
    : UMeshComponent(Other)
    , SkeletalMesh(nullptr)
{
}

UObject* USkinnedMeshComponent::Duplicate() const
{
    USkinnedMeshComponent* NewComp = FObjectFactory::ConstructObjectFrom<USkinnedMeshComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();
    return NewComp;
}

void USkinnedMeshComponent::DuplicateSubObjects(const UObject* Source)
{
    UMeshComponent::DuplicateSubObjects(Source);
}

void USkinnedMeshComponent::PostDuplicate() {}

void USkinnedMeshComponent::SetSkeletalMesh(USkeletalMesh* InSkeletalMesh)
{
    SkeletalMesh = InSkeletalMesh;
    AABB = SkeletalMesh->BoundingBox;
}

void USkinnedMeshComponent::SkinMeshCPU()
{
    // 1) 원본 메시 데이터와 본 데이터를 가져온다
    const FSkeletalMeshData* MeshData = SkeletalMesh->GetMeshData();
    int32 NumVerts = MeshData->Vertices.Num();
    
    // 2) 출력용 스킨드 버텍스 버퍼 크기 맞추기
    SkinnedVertices.SetNum(NumVerts);

    // 3) 각 버텍스별로 CPU 스키닝 계산
    for (int32 i = 0; i < NumVerts; ++i)
    {
        const FSkinnedVertex& SkinnedVertex = MeshData->Vertices[i];
        const FSkinWeight&    SkinWeight   = MeshData->Weights[i];

        FVector accumPos   = FVector::ZeroVector;
        FVector accumNorm  = FVector::ZeroVector;

        for (int32 k = 0; k < 4; ++k)
        {
            int32 boneIdx = SkinWeight.BoneIndices[k];
            float  weight  = SkinWeight.BoneWeights[k];
            if (weight <= 0.f || boneIdx < 0 || boneIdx >= MeshData->Bones.Num())
                continue;

            // UpdateBoneTransforms()에서 갱신된 GlobalReferencePose 사용
            const FBoneInfo& BI = MeshData->Bones[boneIdx];
            // 스킨 매트릭스 = 현재 본 글로벌 트랜스폼 × 인버스 바인드 포즈
            FMatrix SkinningMatrix = BI.GlobalReferencePose * BI.InverseBindPose;

            accumPos  += SkinningMatrix.TransformPosition(SkinnedVertex.Position) * weight;
            accumNorm += FMatrix::TransformVector(SkinnedVertex.Normal, SkinningMatrix) * weight;
        }

        // 결과 저장 (노말은 단위 벡터로 정규화)
        SkinnedVertices[i].Position = accumPos;
        SkinnedVertices[i].Normal   = accumNorm.Normalize();
        SkinnedVertices[i].UV       = SkinnedVertex.UV;
    }
}

void USkinnedMeshComponent::SubmitBuffersToGPU()
{
    // 1) 메시가 세팅되어 있고, 스킨된 버텍스 배열이 비어있지 않은지 체크
    if (!SkeletalMesh || SkinnedVertices.Num() == 0)
        return;

    // 2) 리소스 매니저에서 해당 메시의 동적 VB 가져오기
    FRenderResourceManager* RsrcMgr = FEngineLoop::Renderer.GetResourceManager();
    ID3D11Buffer* VB = RsrcMgr->GetVertexBuffer(SkeletalMesh->GetName());
    if (!VB)
        return; // 버퍼 없으면 종료

    // 3) 맵 → 복사 → 언맵
    auto& DC = FEngineLoop::GraphicDevice.DeviceContext;
    D3D11_MAPPED_SUBRESOURCE Mapped;
    DC->Map(VB, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    std::memcpy(
        Mapped.pData,
        SkinnedVertices.GetData(),
        sizeof(FSkinnedVertex) * SkinnedVertices.Num()
    );
    DC->Unmap(VB, 0);
}



