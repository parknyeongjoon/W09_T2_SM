﻿#include "FinalRenderPass.h"

#include "LaunchEngineLoop.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Renderer/Renderer.h"

FFinalRenderPass::FFinalRenderPass(const FName& InShaderName) : FBaseRenderPass(InShaderName)
{
    bRender = true;
}

void FFinalRenderPass::AddRenderObjectsToRenderPass()
{
}

void FFinalRenderPass::Prepare(FRenderer* Renderer, std::shared_ptr<FViewportClient> InViewportClient, , const FString& InShaderName)
{
    bRender = true;    
    if (bRender)
    {
        FBaseRenderPass::Prepare(Renderer, InViewportClient, InShaderName);
        const FRenderer& Renderer = GEngineLoop.Renderer;
        
        FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;
        Graphics.SwapPingPongBuffers();
        
        Graphics.DeviceContext->OMSetRenderTargets(1, &Graphics.FrameBufferRTV, nullptr);
        Graphics.DeviceContext->OMSetDepthStencilState(Renderer.GetDepthStencilState(EDepthStencilState::DepthNone), 0);

        Graphics.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        ID3D11SamplerState* Sampler = Renderer.GetSamplerState(ESamplerType::Point);
        Graphics.DeviceContext->PSSetSamplers(0, 1, &Sampler);

        const auto PreviousSRV = Graphics.GetPreviousShaderResourceView();
        Graphics.DeviceContext->PSSetShaderResources(0, 1, &PreviousSRV);
    }
}

void FFinalRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;

    if (bRender)
    {
        Graphics.DeviceContext->Draw(6, 0);

        bRender = false;
    }

    Graphics.SwapPingPongBuffers();
}