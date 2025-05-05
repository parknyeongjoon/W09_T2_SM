#include "USkeletalMeshComponent.h"

USkeletalMeshComponent::USkeletalMeshComponent(const USkeletalMeshComponent& Other)
    : USkinnedMeshComponent(Other)
    , bShowBindPose(true)
{
    
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    UpdateBoneTransforms();
    SkinMeshCPU();
    SubmitBuffersToGPU();
}

void USkeletalMeshComponent::UpdateBoneTransforms()
{
}

UObject* USkeletalMeshComponent::Duplicate() const
{
    USkeletalMeshComponent* NewComp = FObjectFactory::ConstructObjectFrom<USkeletalMeshComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();
    return NewComp;
}

void USkeletalMeshComponent::DuplicateSubObjects(const UObject* Source)
{
    USkinnedMeshComponent::DuplicateSubObjects(Source);
}

void USkeletalMeshComponent::PostDuplicate() {}