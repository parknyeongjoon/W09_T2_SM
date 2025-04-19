#include "ShadowRenderPass.h"
#include "Components/LightComponents/PointLightComponent.h"
#include "Components/LightComponents/SpotLightComponent.h"
#include <Renderer/Renderer.h>
#include "EditorEngine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "LevelEditor/SLevelEditor.h"
#include <Math/JungleMath.h>
#include "Engine/World.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Renderer/VBIBTopologyMapping.h"
#include "GameFramework/Actor.h"
#include "Components/Mesh/StaticMesh.h"


FShadowRenderPass::FShadowRenderPass(const FName& InShaderName)
    :FBaseRenderPass(InShaderName)
{
    FGraphicsDevice& Graphics = GEngine->graphicDevice;
    D3D11_BUFFER_DESC cbDesc = {};
    for (int i = 0; i < 8; ++i)
    {
        cbDesc = {};
        cbDesc.ByteWidth = sizeof(FLightCameraConstant);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbDesc.MiscFlags = 0;
        cbDesc.StructureByteStride = 0;
        ID3D11Buffer* ConstantBuffer = nullptr;
        HRESULT hr = Graphics.Device->CreateBuffer(&cbDesc, nullptr, &ConstantBuffer);
        if (FAILED(hr))
        {
            // 에러 처리
        }
        CameraConstantBuffers.Add(ConstantBuffer);
    }
}

void FShadowRenderPass::AddRenderObjectsToRenderPass(UWorld* InWorld)
{
    for (const AActor* actor : InWorld->GetActors())
    {
        for (const UActorComponent* actorComp : actor->GetComponents())
        {
            if (UStaticMeshComponent* pStaticMeshComp = Cast<UStaticMeshComponent>(actorComp))
            {
                if (!Cast<UGizmoBaseComponent>(actorComp))
                    StaticMeshComponents.Add(pStaticMeshComp);
            }

            if (ULightComponentBase* pGizmoComp = Cast<ULightComponentBase>(actorComp))
            {
                Lights.Add(pGizmoComp);
            }
        }
    }
}

void FShadowRenderPass::ClearRenderObjects()
{
    Lights.Empty();
    StaticMeshComponents.Empty();
}

void FShadowRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FBaseRenderPass::Prepare(InViewportClient);

    const FRenderer& Renderer = GEngine->renderer;
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;

    Graphics.DeviceContext->PSSetShader(nullptr, nullptr, 0);
    Graphics.DeviceContext->OMSetDepthStencilState(Renderer.GetDepthStencilState(EDepthStencilState::LessEqual), 0);

    Graphics.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 정정 연결 방식 설정
    Graphics.DeviceContext->RSSetState(Renderer.GetCurrentRasterizerState());
    //D3D11_VIEWPORT vp = {};
    //vp.TopLeftX = 0;
    //vp.TopLeftY = 0;
    //vp.Width = 1024; // ShadowMap size
    //vp.Height = 1024;
    //vp.MinDepth = 0.0f;
    //vp.MaxDepth = 1.0f;
    //Graphics.DeviceContext->RSSetViewports(1, &vp);

    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    Graphics.DeviceContext->PSSetShaderResources(0, 1, nullSRV);
}

void FShadowRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FRenderer& Renderer = GEngine->renderer;
    FGraphicsDevice& Graphics = GEngine->graphicDevice;

    FMatrix Model = FMatrix::Identity;
    FMatrix View = FMatrix::Identity;
    FMatrix Proj = FMatrix::Identity;

    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);
    FMatrix CameraView = curEditorViewportClient->GetViewMatrix();
    FMatrix CameraProjection = curEditorViewportClient->GetProjectionMatrix();
    FFrustum CameraFrustum = FFrustum::ExtractFrustum(CameraView * CameraProjection);

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
    int curLight = 0;
    for (ULightComponentBase* Comp : Lights)
    {
        if (!IsLightInFrustum(Comp, CameraFrustum))
        {
            continue;
        }
        if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(Comp))
        {
            FShadowResource* ShadowResource = SpotLight->GetShadowResource();
            if (ShadowResource == nullptr)
                return;

            D3D11_VIEWPORT vp = ShadowResource->GetViewport();
            Graphics.DeviceContext->RSSetViewports(1, &vp);
            Prepare(InViewportClient);
            Graphics.DeviceContext->ClearDepthStencilView(
                ShadowResource->GetDSV(),
                D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                1.0f, 0
            );
            //Graphics.DeviceContext->ClearRenderTargetView(SpotLight->GetRTV(), ClearColor);
            ID3D11ShaderResourceView* nullSRVs[8] = { nullptr };

            Graphics.DeviceContext->PSSetShaderResources(3, 8, nullSRVs);
            Graphics.DeviceContext->OMSetRenderTargets(0, nullptr, ShadowResource->GetDSV()); // 렌더 타겟 설정
            View = SpotLight->GetViewMatrix();//GEngine->GetLevelEditor()->GetActiveViewportClient()->GetViewMatrix();
            Proj = SpotLight->GetProjectionMatrix();//GEngine->GetLevelEditor()->GetActiveViewportClient()->GetProjectionMatrix();
            for (const auto& StaticMesh : StaticMeshComponents)
            {
                if (!StaticMesh->GetStaticMesh()) continue;
                const OBJ::FStaticMeshRenderData* renderData = StaticMesh->GetStaticMesh()->GetRenderData();
                if (renderData == nullptr) continue;
                Model = StaticMesh->GetWorldMatrix();
                UpdateCameraConstant(Model, View, Proj, curLight);
                // VIBuffer Bind
                const std::shared_ptr<FVBIBTopologyMapping> VBIBTopMappingInfo = Renderer.GetVBIBTopologyMapping(StaticMesh->GetVBIBTopologyMappingName());
                VBIBTopMappingInfo->Bind();

                // If There's No Material Subset
                if (renderData->MaterialSubsets.Num() == 0)
                {
                    Graphics.DeviceContext->DrawIndexed(VBIBTopMappingInfo->GetNumIndices(), 0, 0);
                }

                // SubSet마다 Material Update 및 Draw
                for (int subMeshIndex = 0; subMeshIndex < renderData->MaterialSubsets.Num(); ++subMeshIndex)
                {
                    const int materialIndex = renderData->MaterialSubsets[subMeshIndex].MaterialIndex;

                    // index draw
                    const uint64 startIndex = renderData->MaterialSubsets[subMeshIndex].IndexStart;
                    const uint64 indexCount = renderData->MaterialSubsets[subMeshIndex].IndexCount;
                    Graphics.DeviceContext->DrawIndexed(indexCount, startIndex, 0);
                }
            }
            ID3D11RenderTargetView* RTV = SpotLight->GetRTV();
            GEngine->renderer.PrepareShader(TEXT("LightDepth"));
            Graphics.DeviceContext->OMSetRenderTargets(1, &RTV, nullptr); // 렌더 타겟 설정
            Graphics.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            ID3D11SamplerState* Sampler = Renderer.GetSamplerState(ESamplerType::Point);
            Graphics.DeviceContext->PSSetSamplers(0, 1, &Sampler);
            ID3D11ShaderResourceView* ShadowMap = ShadowResource->GetSRV();
            Graphics.DeviceContext->CopyResource(Graphics.DepthCopyTexture, Graphics.DepthStencilBuffer);
            Graphics.DeviceContext->PSSetShaderResources(0, 1, &ShadowMap);
            Graphics.DeviceContext->Draw(4, 0);
            curLight += 1;
        }
            else if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(Comp))
            {
                Prepare(InViewportClient);
                // 모든 면의 DSV를 한 번에 클리어
                Graphics.DeviceContext->ClearDepthStencilView(
                    PointLight->GetShadowResource()->GetDSV(),
                    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                    1.0f, 0
                );

                // 6개 면에 대해 각각 렌더링
                for (int face = 0; face < 6; face++)
                {
                    // 현재 면의 DSV 가져오기
                    ID3D11DepthStencilView* faceDSV = PointLight->GetFaceDSV(face);

                    // 셰이더 리소스 초기화
                    ID3D11ShaderResourceView* nullSRVs[8] = { nullptr };
                    Graphics.DeviceContext->PSSetShaderResources(3, 8, nullSRVs);

                    // 렌더 타겟 설정
                    Graphics.DeviceContext->OMSetRenderTargets(0, nullptr, faceDSV);

                    // 현재 면의 뷰/투영 행렬 설정
                    View = PointLight->GetViewMatrixForFace(face);
                    Proj = PointLight->GetProjectionMatrix(); // 90도 FOV 투영 행렬

                    // 모든 메시 렌더링
                    for (const auto& StaticMesh : StaticMeshComponents)
                    {
                        if (!StaticMesh->GetStaticMesh()) continue;
                        const OBJ::FStaticMeshRenderData* renderData = StaticMesh->GetStaticMesh()->GetRenderData();
                        if (renderData == nullptr) continue;

                        Model = StaticMesh->GetWorldMatrix();
                        // 현재 면과 라이트 인덱스 정보 업데이트
                        UpdateCameraConstant(Model, View, Proj, curLight);

                        // VIBuffer Bind
                        const std::shared_ptr<FVBIBTopologyMapping> VBIBTopMappingInfo =
                            Renderer.GetVBIBTopologyMapping(StaticMesh->GetVBIBTopologyMappingName());
                        VBIBTopMappingInfo->Bind();

                        // 서브셋 렌더링
                        if (renderData->MaterialSubsets.Num() == 0)
                        {
                            Graphics.DeviceContext->DrawIndexed(VBIBTopMappingInfo->GetNumIndices(), 0, 0);
                        }
                        else
                        {
                            for (int subMeshIndex = 0; subMeshIndex < renderData->MaterialSubsets.Num(); ++subMeshIndex)
                            {
                                const uint64 startIndex = renderData->MaterialSubsets[subMeshIndex].IndexStart;
                                const uint64 indexCount = renderData->MaterialSubsets[subMeshIndex].IndexCount;
                                Graphics.DeviceContext->DrawIndexed(indexCount, startIndex, 0);
                            }
                        }
                    }
                }

                // 후처리 (필요한 경우)
                ID3D11RenderTargetView* RTV = PointLight->GetRTV();
                if (RTV != nullptr)
                {// 각 면의 개별 SRV 생성
                    for (int face = 0; face < 6; face++)
                    {
                        // 각 면의 개별 SRV 생성
                        D3D11_SHADER_RESOURCE_VIEW_DESC faceDesc = {};
                        faceDesc.Format = DXGI_FORMAT_R32_FLOAT;
                        faceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                        faceDesc.Texture2DArray.MostDetailedMip = 0;
                        faceDesc.Texture2DArray.MipLevels = 1;
                        faceDesc.Texture2DArray.FirstArraySlice = face;  // 현재 면 선택
                        faceDesc.Texture2DArray.ArraySize = 1;

                        ID3D11ShaderResourceView* faceSRV = nullptr;
                        HRESULT hr = Graphics.Device->CreateShaderResourceView(
                            PointLight->GetShadowResource()->GetTexture(),
                            &faceDesc,
                            &faceSRV
                        );

                        if (SUCCEEDED(hr))
                        {
                            GEngine->renderer.PrepareShader(TEXT("LightDepth"));
                            Graphics.DeviceContext->OMSetRenderTargets(1, &RTV, nullptr);
                            Graphics.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                            ID3D11SamplerState* Sampler = Renderer.GetSamplerState(ESamplerType::Point);
                            Graphics.DeviceContext->PSSetSamplers(0, 1, &Sampler);

                            Graphics.DeviceContext->CopyResource(Graphics.DepthCopyTexture, Graphics.DepthStencilBuffer);
                            Graphics.DeviceContext->PSSetShaderResources(0, 1, &faceSRV);
                            Graphics.DeviceContext->Draw(4, 0);

                            // 리소스 해제
                            faceSRV->Release();
                        }
                    }
                }

                curLight += 1;
            }
      
    }
    Graphics.DeviceContext->RSSetViewports(1, &curEditorViewportClient->GetD3DViewport());
    Graphics.DeviceContext->OMSetRenderTargets(1, &Graphics.RTVs[0], Graphics.DepthStencilView);
}

void FShadowRenderPass::UpdateCameraConstant(FMatrix Model, FMatrix View, FMatrix Proj, int index)
{
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;
    FRenderResourceManager* renderResourceManager = GEngine->renderer.GetResourceManager();

    FLightCameraConstant CameraConstants;
    CameraConstants.Model = Model;
    CameraConstants.View = View;
    CameraConstants.Proj = Proj;

    renderResourceManager->UpdateConstantBuffer(CameraConstantBuffers[index], &CameraConstants);

    Graphics.DeviceContext->VSSetConstantBuffers(0, 1, &CameraConstantBuffers[index]);
}



bool FShadowRenderPass::IsLightInFrustum(ULightComponentBase* LightComponent, const FFrustum& CameraFrustum) const
{
    // if (dynamic_cast<UDirectionalLightComponent*>(LightComponent) && !dynamic_cast<USpotLightComponent>(LightComponent))
    // {
    //     return true;
    // }

    // 포인트 라이트 : 구 형태 판단
    if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(LightComponent))
    {
        FVector LightCenter = PointLightComp->GetComponentLocation();
        float Radius = PointLightComp->GetRadius();
        return CameraFrustum.IntersectsSphere(LightCenter, Radius);
    }

    // 스팟 라이트의 경우, 보통 구 또는 원뿔의 바운딩 볼륨을 사용합니다.
    if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(LightComponent))
    {
        // FVector LightCenter = SpotLightComp->GetComponentLocation();
        // // 스팟 라이트의 영향을 대략적으로 표현하는 반지름 (필요 시 실제 cone 계산으로 대체)
        // float ApproxRadius = SpotLightComp->GetOuterConeAngle(); // 예시: cone 각도를 사용 (단위 및 스케일은 조정 필요)
        // return CameraFrustum.IntersectsSphere(LightCenter, ApproxRadius);
        return IsSpotLightInFrustum(SpotLightComp, CameraFrustum);
    }

    // 그 외의 경우, 보수적으로 true로 반환
    return true;
}

bool FShadowRenderPass::IsSpotLightInFrustum(USpotLightComponent* SpotLightComp, const FFrustum& CameraFrustum) const
{
    // 스팟 라이트의 Apex(위치)
    FVector Apex = SpotLightComp->GetComponentLocation();

    // 스팟 라이트의 방향: 스팟 라이트의 오너의 전방벡터를 사용 (정규화된 값)
    FVector Dir = SpotLightComp->GetOwner()->GetActorForwardVector().Normalize();

    // 스팟 라이트의 범위 (거리) - 일반적으로 Attenuation Radius 또는 Range를 사용
    float Range = 50;

    // 스팟 라이트의 외부 콘 각도 (단위: 도)를 라디안으로 변환
    float OuterAngleRad = SpotLightComp->GetOuterConeAngle();

    // 원뿔의 베이스(밑면) 중심과 반지름 계산
    FVector BaseCenter = Apex + Dir * Range;
    float BaseRadius = Range * FMath::Tan(OuterAngleRad);

    // 1. Apex(꼭짓점)가 프러스텀 내부에 있으면 전체 원뿔도 영향을 줄 가능성이 높으므로 true
    if (CameraFrustum.IntersectsPoint(Apex))
    {
        return true;
    }

    // 2. 베이스 중심이 프러스텀 내부에 있으면 true
    if (CameraFrustum.IntersectsPoint(BaseCenter))
    {
        return true;
    }

    // 3. 베이스 원의 둘레를 여러 샘플링하여 프러스텀 내부 포함 여부 검사
    //    (정확도를 높이기 위해 샘플 수를 늘릴 수 있습니다)
    const int SampleCount = 8;  // 예제에서는 8개의 점으로 샘플링
    // 원뿔 베이스의 평면에 대한 임의의 좌표계를 생성
    FVector Right = Dir.Cross(FVector(0, 1, 0));
    if (Right.IsNearlyZero())  // 만약 Dir이 (0,1,0)와 평행하면 다른 벡터로 교차
    {
        Right = Dir.Cross(FVector(1, 0, 0));
    }
    Right.Normalize();
    FVector Up = Dir.Cross(Right).Normalize();

    for (int i = 0; i < SampleCount; ++i)
    {
        float Angle = (2.f * PI * i) / SampleCount;
        // 베이스 원의 둘레 상의 샘플 포인트 계산
        FVector Offset = (Right * FMath::Cos(Angle) + Up * FMath::Sin(Angle)) * BaseRadius;
        FVector SamplePoint = BaseCenter + Offset;

        // 샘플 포인트가 프러스텀 내부에 있으면 스팟 라이트 영향 영역이 프러스텀에 일부 포함된 것으로 판단
        if (CameraFrustum.IntersectsPoint(SamplePoint))
        {
            return true;
        }
    }

    // 모든 검사에서 프러스텀 내부에 포함된 점이 없으면 false
    return false;
}
