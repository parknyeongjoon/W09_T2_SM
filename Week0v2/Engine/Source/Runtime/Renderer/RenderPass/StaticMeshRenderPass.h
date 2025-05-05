#pragma once
#include "Define.h"
#include "FBaseRenderPass.h"
#include "Container/Array.h"
#include "Math/Vector4.h"

class USpotLightComponent;
class ULightComponentBase;
class USkySphereComponent;
struct FObjMaterialInfo;
struct FMatrix;
class UStaticMeshComponent;

class FStaticMeshRenderPass : public FBaseRenderPass
{
public:
    explicit FStaticMeshRenderPass(const FName& InShaderName);

    virtual ~FStaticMeshRenderPass() {}
    void AddRenderObjectsToRenderPass() override;
    void Prepare(std::shared_ptr<FViewportClient> InViewportClient) override;
    void UpdateComputeResource();
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;
    //void UpdateComputeConstants(std::shared_ptr<FViewportClient> InViewportClient);
    void CreateDummyTexture();

    void ClearRenderObjects() override;
private:
    static void UpdateMatrixConstants(UStaticMeshComponent* InStaticMeshComponent, const FMatrix& InView, const FMatrix& InProjection);
    void UpdateFlagConstant();
    void UpdateLightConstants();
    // void UpdateContstantBufferActor(const FVector4 UUID, int32 isSelected);
    static void UpdateSkySphereTextureConstants(const USkySphereComponent* InSkySphereComponent);
    static void UpdateMaterialConstants(const FObjMaterialInfo& MaterialInfo);
    void UpdateCameraConstant(const std::shared_ptr<FViewportClient>& InViewportClient);

private:
    bool IsLightInFrustum(ULightComponentBase* LightComponent, const FFrustum& CameraFrustum) const;
    bool IsSpotLightInFrustum(USpotLightComponent* SpotLightComp, const FFrustum& CameraFrustum) const;
private:
    TArray<ULightComponentBase*> LightComponents;
    TArray<UStaticMeshComponent*> StaticMesheComponents;
    ID3D11ShaderResourceView* DummyWhiteTextureSRV = nullptr;
    ID3D11SamplerState* ShadowMapSampler = nullptr;
    ID3D11Buffer* LightConstantBuffer = nullptr;
};
