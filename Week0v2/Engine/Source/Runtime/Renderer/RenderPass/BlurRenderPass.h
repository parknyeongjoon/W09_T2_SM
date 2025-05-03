﻿#pragma once
#include "FBaseRenderPass.h"

class FBlurRenderPass : public FBaseRenderPass
{
public:
    explicit FBlurRenderPass(const FName& InShaderName);

    virtual ~FBlurRenderPass() {}
    void AddRenderObjectsToRenderPass() override;
    void Prepare(FRenderer* Renderer, std::shared_ptr<FViewportClient> InViewportClient, const FString& InShaderName = FString("")) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;
    bool bRender;

    class ID3D11Buffer* BlurConstantBuffer = nullptr;

    void UpdateBlurConstant(float BlurStrength, float TexelX, float TexelY) const;
};
