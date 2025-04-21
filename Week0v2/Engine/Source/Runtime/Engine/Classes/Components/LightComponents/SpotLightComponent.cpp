#include "SpotLightComponent.h"
#include "UObject/ObjectFactory.h"
#include "CoreUObject/UObject/Casts.h"
#include "Math/JungleMath.h"
#include <D3D11RHI/GraphicDevice.h>
#include "EditorEngine.h"

USpotLightComponent::USpotLightComponent()
    : Super()
{
    ShadowResource = FShadowResourceFactory::CreateShadowResource(GEngine->graphicDevice.Device, ELightType::SpotLight);
    // FGraphicsDevice& Graphics = GEngine->graphicDevice;
    //
    // LightMap = new FTexture(nullptr, nullptr, 0, 0, L"");
    //
    // D3D11_TEXTURE2D_DESC textureDesc = {};
    // textureDesc.Width = FShadowResource::ShadowResolution;
    // textureDesc.Height = FShadowResource::ShadowResolution;
    // textureDesc.MipLevels = 1;
    // textureDesc.ArraySize = 1;
    // textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    // textureDesc.SampleDesc.Count = 1;
    // textureDesc.Usage = D3D11_USAGE_DEFAULT;
    // textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    //
    // HRESULT hr = Graphics.Device->CreateTexture2D(&textureDesc, nullptr, &LightMap->Texture);
    // if (FAILED(hr))
    // {
    //     assert(TEXT("SceneColorBuffer creation failed"));
    //     return;
    // }
    //
    // D3D11_RENDER_TARGET_VIEW_DESC SceneColorRTVDesc = {};
    // SceneColorRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;      // 색상 포맷
    // SceneColorRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처
    //
    // hr = Graphics.Device->CreateRenderTargetView(LightMap->Texture, &SceneColorRTVDesc, &LightRTV);
    // if (FAILED(hr))
    // {
    //     assert(TEXT("SceneColorBuffer creation failed"));
    //     return;
    // }
    //
    // D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    // srvDesc = {};
    // srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    // srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    // srvDesc.Texture2D.MostDetailedMip = 0;
    // srvDesc.Texture2D.MipLevels = 1;
    //
    // hr = Graphics.Device->CreateShaderResourceView(LightMap->Texture, &srvDesc, &LightMap->TextureSRV);
    // if (FAILED(hr))
    // {
    //     assert(TEXT("SceneColorBuffer creation failed"));
    //     return;
    // }
}

USpotLightComponent::USpotLightComponent(const USpotLightComponent& Other)
    : Super(Other)
    , InnerConeAngle(Other.InnerConeAngle)
    , OuterConeAngle(Other.OuterConeAngle)
    , Direction(Other.Direction)
{

}

USpotLightComponent::~USpotLightComponent()
{

}

void USpotLightComponent::SetInnerConeAngle(float Angle)
{
    Angle = JungleMath::DegToRad(Angle);
    InnerConeAngle = Angle;
}

void USpotLightComponent::SetOuterConeAngle(float Angle)
{
    Angle = JungleMath::DegToRad(Angle);
    OuterConeAngle = Angle;
}

UObject* USpotLightComponent::Duplicate() const
{
    USpotLightComponent* NewComp = FObjectFactory::ConstructObjectFrom<USpotLightComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();

    return NewComp;
}

void USpotLightComponent::DuplicateSubObjects(const UObject* Source)
{
    Super::DuplicateSubObjects(Source);
}

void USpotLightComponent::PostDuplicate()
{
}

FMatrix USpotLightComponent::GetViewMatrix() const
{
    FVector Up = FVector{ 0.0f, 0.0f, 1.0f };
    FVector Forward = GetForwardVector();
    float dot = abs(Up.Dot(Forward));
    if (dot > 0.99f)
        Up = FVector(1.0f, 0.0f, 0.0f);
    return JungleMath::CreateViewMatrix(GetComponentLocation(), Forward + GetComponentLocation(), Up);
} 

FMatrix USpotLightComponent::GetProjectionMatrix() const
{
    return JungleMath::CreateProjectionMatrix(
        OuterConeAngle * 2.0f,
        1.0f,
        0.1f,
        1000.0f
    );
}

std::shared_ptr<FActorComponentInfo> USpotLightComponent::GetActorComponentInfo()
{
    std::shared_ptr<FSpotlightComponentInfo> Info = std::make_shared<FSpotlightComponentInfo>();
    Super::GetActorComponentInfo()->Copy(*Info);

    Info->InnerConeAngle = InnerConeAngle;
    Info->OuterConeAngle = OuterConeAngle;

    return Info;
}

void USpotLightComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);
    const FSpotlightComponentInfo& PointLightInfo = static_cast<const FSpotlightComponentInfo&>(Info);
    InnerConeAngle = PointLightInfo.InnerConeAngle;
    OuterConeAngle = PointLightInfo.OuterConeAngle;
}