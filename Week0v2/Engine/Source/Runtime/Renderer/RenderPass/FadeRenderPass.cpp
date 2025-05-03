#include "FadeRenderPass.h"

#include "PlayerCameraManager.h"
#include "EditorEngine.h"
#include "LaunchEngineLoop.h"
#include "D3D11RHI/CBStructDefine.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Engine/World.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderResourceManager.h"
#include "UObject/UObjectIterator.h"

extern UEngine* GEngine;

FFadeRenderPass::FFadeRenderPass(const FName& InShaderName)
    : FBaseRenderPass(InShaderName)
{

    FRenderer& Renderer = GEngineLoop.Renderer;
    FRenderResourceManager* RenderResourceManager = Renderer.GetResourceManager();
    bRender = true;
    FadeConstantBuffer = RenderResourceManager->CreateConstantBuffer(sizeof(FFadeConstants));
}

void FFadeRenderPass::AddRenderObjectsToRenderPass()
{
    for (APlayerCameraManager* PlayerCameraManager : TObjectRange<APlayerCameraManager>())
    {
        if (PlayerCameraManager->GetWorld() != GEngine->GetWorld())
        {
            continue;
        }
        PlayerCameraManagers.Add(PlayerCameraManager);
    }
}

void FFadeRenderPass::Prepare(FRenderer* Renderer, std::shared_ptr<FViewportClient> InViewportClient, const FString& InShaderName)
{
    bRender = true;
    
    if (bRender)
    {
        FBaseRenderPass::Prepare(Renderer, InViewportClient, InShaderName);

        FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;
        Graphics.SwapPingPongBuffers();

        const auto CurRTV = Graphics.GetCurrentRenderTargetView();
        Graphics.DeviceContext->OMSetRenderTargets(1, &CurRTV, nullptr);
        Graphics.DeviceContext->OMSetDepthStencilState(Renderer->GetDepthStencilState(EDepthStencilState::DepthNone), 0);

        Graphics.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        ID3D11SamplerState* Sampler = Renderer->GetSamplerState(ESamplerType::Linear);
        Graphics.DeviceContext->PSSetSamplers(0, 1, &Sampler);

        const auto PreviousSRV = Graphics.GetPreviousShaderResourceView();
        Graphics.DeviceContext->PSSetShaderResources(1, 1, &PreviousSRV);
    }
}

void FFadeRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{

    if (bRender)
    {
        UpdateFadeConstant();
        FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;
        Graphics.DeviceContext->Draw(6, 0);

        bRender = false;
    }
}

void FFadeRenderPass::ClearRenderObjects()
{
    PlayerCameraManagers.Empty();
}

void FFadeRenderPass::UpdateFadeConstant()
{

    FFadeConstants FadeConstants;
    FLinearColor AccumulatedFadeColor = {0.f,0.f,0.f,0.f}; // 시작은 완전 투명 (알파 0)
    float AccumulatedFadeAlpha = 0.0f;
    
    for (APlayerCameraManager* iter : PlayerCameraManagers)
    {
        const TArray<FPostProcessSettings>* PPSettingsPtr = nullptr;
        const TArray<float>* BlendWeightsPtr = nullptr;
        iter->GetCachedPostProcessBlends(PPSettingsPtr, BlendWeightsPtr);
        
        
        if (PPSettingsPtr )
        {
            int32 NumSettings = PPSettingsPtr->Num();
            for (int i = 0 ; i < NumSettings; ++i)
            {
                // 현재 레이어(i)의 설정과 가중치 가져오기
                const FPostProcessSettings& LayerSettings = (*PPSettingsPtr)[i];
                const float LayerWeight = 1.0f; // 이 모디파이어/설정의 전체 영향력

                // 현재 레이어의 유효 알파 계산 (가중치 * 설정 내 알파)
                // FadeAlpha가 0~1 범위라고 가정, LayerWeight도 0~1 범위라고 가정
                const float EffectiveLayerAlpha = FMath::Clamp(LayerWeight * LayerSettings.FadeAlpha, 0.0f, 1.0f);

                // 현재 레이어의 페이드 색상
                const FLinearColor LayerFadeColor = LayerSettings.FadeColor;

                // --- 알파 블렌딩 ("Over" 연산) ---
                // NewColor = ForegroundColor * ForegroundAlpha + BackgroundColor * (1 - ForegroundAlpha)
                // NewAlpha = ForegroundAlpha + BackgroundAlpha * (1 - ForegroundAlpha)
                // 여기서 Foreground = 현재 레이어(Layer), Background = 이전까지 누적된 결과(Accumulated)

                // 1. 새로운 누적 색상 계산
                //    현재 레이어 색상의 기여분 + 이전 누적 색상의 기여분 (현재 레이어를 통과한 만큼)
                FLinearColor NewAccumulatedColor = (LayerFadeColor * EffectiveLayerAlpha) + (AccumulatedFadeColor * (1.0f - EffectiveLayerAlpha));

                // 2. 새로운 누적 알파 계산
                //    현재 레이어 알파 + 이전 누적 알파 (현재 레이어를 통과한 만큼)
                float NewAccumulatedAlpha = EffectiveLayerAlpha + (AccumulatedFadeAlpha * (1.0f - EffectiveLayerAlpha));

                // 3. 누적 값 업데이트
                AccumulatedFadeColor = NewAccumulatedColor;
                AccumulatedFadeAlpha = FMath::Clamp(NewAccumulatedAlpha, 0.0f, 1.0f); // 누적 알파도 0~1 범위 유지
            }
        }
    }

    FadeConstants.FadeColor = AccumulatedFadeColor;
    FadeConstants.FadeColor.A = 1.0f; // 알파는 1로 고정 (페이드 색상은 완전 불투명)
    FadeConstants.FadeAlpha = AccumulatedFadeAlpha;

    if (FadeConstants.FadeAlpha >0.1f)
    {
        int a = 0;
    }
        


    const FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;
    FRenderResourceManager* renderResourceManager = GEngineLoop.Renderer.GetResourceManager();

    renderResourceManager->UpdateConstantBuffer(FadeConstantBuffer, &FadeConstants);
    Graphics.DeviceContext->PSSetConstantBuffers(0, 1, &FadeConstantBuffer);

        
}

// void FFadeRenderPass::UpdateFadeConstant()
// {
//     const FGraphicsDevice& Graphics = GEngine->graphicDevice;
//     FRenderResourceManager* renderResourceManager = GEngine->renderer.GetResourceManager();
//     std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);
//
//     FFogCameraConstant CameraConstants;
//     CameraConstants.InvProjMatrix = FMatrix::Inverse(curEditorViewportClient->GetProjectionMatrix());
//     CameraConstants.InvViewMatrix = FMatrix::Inverse(curEditorViewportClient->GetViewMatrix());
//     CameraConstants.CameraForward = curEditorViewportClient->ViewTransformPerspective.GetForwardVector();
//     CameraConstants.CameraPos = curEditorViewportClient->ViewTransformPerspective.GetLocation();
//     CameraConstants.NearPlane = curEditorViewportClient->GetNearClip();
//     CameraConstants.FarPlane = curEditorViewportClient->GetFarClip();
//
//     renderResourceManager->UpdateConstantBuffer(FadeConstantBuffer, &CameraConstants);
//     Graphics.DeviceContext->PSSetConstantBuffers(0, 1, &FadeConstantBuffer);
// }
