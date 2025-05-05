#pragma once
#include "Components/Mesh/USkeletalMesh.h"
#include "Components/PrimitiveComponents/MeshComponents/MeshComponent.h"

class USkinnedMeshComponent : public UMeshComponent
{
    DECLARE_CLASS(USkinnedMeshComponent, UMeshComponent)
    
public:
    USkinnedMeshComponent() = default;
    USkinnedMeshComponent(const USkinnedMeshComponent& Other);

    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;

    void SetSkeletalMesh(USkeletalMesh* InSkeletalMesh);
    // CPU 스키닝 알고리즘
    void SkinMeshCPU();
    // Dynamic VB 갱신
    void SubmitBuffersToGPU();
    
protected:
    USkeletalMesh* SkeletalMesh;
    // CPU 스키닝 후 결과 버텍스 배열
    TArray<FSkinnedVertex> SkinnedVertices;

};
