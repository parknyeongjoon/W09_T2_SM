#pragma once
#include "USkinnedMeshComponent.h"

class USkeletalMeshComponent : public USkinnedMeshComponent
{
    DECLARE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)
    
public:
    bool bShowBindPose;
    
public:
    USkeletalMeshComponent() = default;
    USkeletalMeshComponent(const USkeletalMeshComponent& Other);
    
    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;
    virtual void TickComponent(float DeltaTime) override;

    bool IsSetSkeletalMesh() const { return SkeletalMesh != nullptr; }
    USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }

    void UpdateBoneTransforms();
};