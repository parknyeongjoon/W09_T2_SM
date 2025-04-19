#include "PointLightComponent.h"
#include "UObject/ObjectFactory.h"
#include "CoreUObject/UObject/Casts.h"
#include "EditorEngine.h"
#include <Math/JungleMath.h>

UPointLightComponent::UPointLightComponent()
{
    ShadowResource = FShadowResourceFactory::CreateShadowResource(GEngine->graphicDevice.Device, ELightType::PointLight);
    LightMap = new FTexture(nullptr, nullptr, 0, 0, L"");

    FGraphicsDevice& Graphics = GEngine->graphicDevice;

  

    //// 2. 깊이 스텐실 뷰 생성 (모든 면에 대해 하나)
    //D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    //dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    //dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    //dsvDesc.Texture2DArray.MipSlice = 0;
    //dsvDesc.Texture2DArray.FirstArraySlice = 0;
    //dsvDesc.Texture2DArray.ArraySize = 6;  // 모든 면 포함

    //hr = Graphics.Device->CreateDepthStencilView(ShadowMap->Texture, &dsvDesc, &DSV);
    //if (FAILED(hr))
    //{
    //    assert(TEXT("ShadowCubeMap DSV creation failed"));
    //    return;
    //}

   

    // 4. 결과 저장을 위한 렌더 타겟 텍스처 생성
    D3D11_TEXTURE2D_DESC lightMapDesc = {};
    lightMapDesc.Width = 1024;
    lightMapDesc.Height = 1024;
    lightMapDesc.MipLevels = 1;
    lightMapDesc.ArraySize = 1;
    lightMapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    lightMapDesc.SampleDesc.Count = 1;
    lightMapDesc.SampleDesc.Quality = 0;
    lightMapDesc.Usage = D3D11_USAGE_DEFAULT;
    lightMapDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    lightMapDesc.CPUAccessFlags = 0;
    lightMapDesc.MiscFlags = 0;

    HRESULT hr = Graphics.Device->CreateTexture2D(&lightMapDesc, nullptr, &LightMap->Texture);
    if (FAILED(hr))
    {
        assert(TEXT("LightMap creation failed"));
        return;
    }

    // 5. 렌더 타겟 뷰 생성
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    hr = Graphics.Device->CreateRenderTargetView(LightMap->Texture, &rtvDesc, &LightRTV);
    if (FAILED(hr))
    {
        assert(TEXT("LightMap RTV creation failed"));
        return;
    }

    // 6. LightMap SRV 생성
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = Graphics.Device->CreateShaderResourceView(LightMap->Texture, &srvDesc, &LightMap->TextureSRV);
    if (FAILED(hr))
    {
        assert(TEXT("LightMap SRV creation failed"));
        return;
    }

  
}

UPointLightComponent::UPointLightComponent(const UPointLightComponent& Other)
    : Super(Other)
    , Radius(Other.Radius)
    , AttenuationFalloff(Other.AttenuationFalloff)
{
}

UObject* UPointLightComponent::Duplicate() const
{
    UPointLightComponent* NewComp = FObjectFactory::ConstructObjectFrom<UPointLightComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();

    return NewComp;
}

void UPointLightComponent::DuplicateSubObjects(const UObject* Source)
{
    Super::DuplicateSubObjects(Source);
}

void UPointLightComponent::PostDuplicate()
{
}

std::shared_ptr<FActorComponentInfo> UPointLightComponent::GetActorComponentInfo()
{
    std::shared_ptr<FPointLightComponentInfo> Info = std::make_shared<FPointLightComponentInfo>();
    Super::GetActorComponentInfo()->Copy(*Info);

    Info->Radius = Radius;
    Info->AttenuationFalloff = AttenuationFalloff;

    return Info;
}

void UPointLightComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);
    const FPointLightComponentInfo& PointLightInfo = static_cast<const FPointLightComponentInfo&>(Info);
    Radius = PointLightInfo.Radius;
    AttenuationFalloff = PointLightInfo.AttenuationFalloff;
}


FMatrix UPointLightComponent::GetViewMatrixForFace(int faceIndex)
{
    FVector Up, Forward;

    // 각 면에 따른 방향 벡터 설정
    switch (faceIndex)
    {
    case 0: // +X 방향
        Forward = FVector(1.0f, 0.0f, 0.0f);
        Up = FVector(0.0f, 0.0f, 1.0f);
        break;
    case 1: // -X 방향
        Forward = FVector(-1.0f, 0.0f, 0.0f);
        Up = FVector(0.0f, 0.0f, 1.0f);
        break;
    case 2: // +Y 방향
        Forward = FVector(0.0f, 1.0f, 0.0f);
        Up = FVector(0.0f, 0.0f, 1.0f);
        break;
    case 3: // -Y 방향
        Forward = FVector(0.0f, -1.0f, 0.0f);
        Up = FVector(0.0f, 0.0f, 1.0f);
        break;
    case 4: // +Z 방향
        Forward = FVector(0.0f, 0.0f, 1.0f);
        Up = FVector(0.0f, 1.0f, 0.0f);
        break;
    case 5: // -Z 방향
        Forward = FVector(0.0f, 0.0f, -1.0f);
        Up = FVector(0.0f, 1.0f, 0.0f);
        break;
    }

    // 위 벡터와 전방 벡터가 평행한지 확인
    float dot = fabs(Up.Dot(Forward));
    if (dot > 0.99f)
    {
        // 평행하면 다른 Up 벡터 사용
        if (faceIndex == 4 || faceIndex == 5) // +Z 또는 -Z 방향
            Up = FVector(1.0f, 0.0f, 0.0f);
        else
            Up = FVector(0.0f, 0.0f, 1.0f);
    }

    return JungleMath::CreateViewMatrix(
        GetComponentLocation(),
        Forward + GetComponentLocation(),
        Up
    );
}

FMatrix UPointLightComponent::GetProjectionMatrix()
{
    // 포인트 라이트는 항상 90도(큐브맵의 각 면)
    const float cubeFaceAngle = 90.0f;

    // 1:1 종횡비 (정사각형)
    const float aspectRatio = 1.0f;

    // 근거리/원거리 평면
    const float nearPlane = 0.1f;
    const float farPlane = 1000.f;

    return JungleMath::CreateProjectionMatrix(
        cubeFaceAngle,
        aspectRatio,
        nearPlane,
        farPlane
    );
}
