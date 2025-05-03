#pragma once
#include "FBaseRenderPass.h"
#include "Windows/D3D11RHI/GraphicDevice.h"

class FDebugDepthRenderPass : public FBaseRenderPass
{
public:
    explicit FDebugDepthRenderPass(const FName& InShaderName)
        :FBaseRenderPass(InShaderName)
    {}

    virtual ~FDebugDepthRenderPass() {}
	void AddRenderObjectsToRenderPass() override;
    void Prepare(FRenderer* Renderer, std::shared_ptr<FViewportClient> InViewportClient, const FString& InShaderName = FString("")) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;

private:
	void UpdateCameraConstant(const std::shared_ptr<FViewportClient> InViewportClient);
    void UpdateScreenConstant(std::shared_ptr<FViewportClient> InViewportClient);
};
