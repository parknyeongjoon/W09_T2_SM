#pragma once
#include "Define.h"
#include "FBaseRenderPass.h"
#include "Components/PrimitiveComponents/MeshComponents/USkeletalMeshComponent.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"

class FSkeletalMeshRenderPass : public FBaseRenderPass
{
public:
    explicit FSkeletalMeshRenderPass(const FName& InShaderName);

    virtual ~FSkeletalMeshRenderPass() {}
    void AddRenderObjectsToRenderPass() override;
    void Prepare(std::shared_ptr<FViewportClient> InViewportClient) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;
    void ClearRenderObjects() override;

    void CreateConstant();
    void UpdateMatrixConstants(USkeletalMeshComponent* InSkeletalMeshComponent, const FMatrix& InView,
                               const FMatrix& InProjection);
    void UpdateConstant();

private:
    bool bRender;
    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
};
