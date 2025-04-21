#pragma once
#include "FBaseRenderPass.h"
#include "Windows/D3D11RHI/GraphicDevice.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include <Components/LightComponents/SpotLightComponent.h>

struct FLightCameraConstant
{
    FMatrix Model;
    FMatrix View;
    FMatrix Proj;
};

class FShadowRenderPass : public FBaseRenderPass
{
public:
    explicit FShadowRenderPass(const FName& InShaderName);

    virtual ~FShadowRenderPass() {}
    void AddRenderObjectsToRenderPass(UWorld* InWorld) override;
    void ClearRenderObjects() override;

    void Prepare(std::shared_ptr<FViewportClient> InViewportClient) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;
    bool ShouldRender() { return bRender; }

private:
    bool IsLightInFrustum(ULightComponentBase* LightComponent, const FFrustum& CameraFrustum) const;
    bool IsSpotLightInFrustum(USpotLightComponent* SpotLightComp, const FFrustum& CameraFrustum) const;
    void RenderStaticMesh(FMatrix View, FMatrix Projection);

private:
    TArray<ULightComponentBase*> Lights;
    TArray<UStaticMeshComponent*> StaticMeshComponents;
    void UpdateCameraConstant(FMatrix Model, FMatrix View, FMatrix Proj) const;
    bool bRender = false;
    ID3D11Buffer* CameraConstantBuffer;
};