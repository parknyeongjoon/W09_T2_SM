#include "ShadowResource.h"
#include "Math/MathUtility.h"
FShadowResource::FShadowResource(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution)
    :LightType(LightType)
{
    switch (LightType)
    {
    case ELightType::DirectionalLight:
    case ELightType::SpotLight:
    {
        // Texture2D 생성
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = ShadowResolution;
        textureDesc.Height = ShadowResolution;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R32_TYPELESS; // 중요: TYPELESS
        textureDesc.MiscFlags = 0;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = Device->CreateTexture2D(&textureDesc, nullptr, &ShadowTexture);
        if (FAILED(hr))
        {
            assert(TEXT("Shadow Texture creation failed"));
            return;
        }
        // SRV
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // 깊이 데이터만 읽기 위한 포맷
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = Device->CreateShaderResourceView(ShadowTexture.Get(), &srvDesc, &ShadowSRV);
        if (FAILED(hr))
        {
            assert(TEXT("Shadow SRV creation failed"));
            return;
        }

        // DSV
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        ComPtr<ID3D11DepthStencilView> dsv;
        hr = Device->CreateDepthStencilView(ShadowTexture.Get(), &dsvDesc, &dsv);
        if (FAILED(hr))
        {
            assert(TEXT("Shadow DSV creation failed"));
            return;
        }
        ShadowDSVs.Add(dsv);

        // Viewport 설정
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = static_cast<FLOAT>(ShadowResolution);
        viewport.Height = static_cast<FLOAT>(ShadowResolution);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        Viewports.Add(viewport);
        break;
    }
    case ELightType::PointLight:
    {
        NumFaces = 6;
        // Texture2D - Cube Array
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = ShadowResolution;
        texDesc.Height = ShadowResolution;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 6;
        texDesc.Format = DXGI_FORMAT_D32_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        HRESULT hr = Device->CreateTexture2D(&texDesc, nullptr, &ShadowTexture);
        if (FAILED(hr))
        {
            assert(TEXT("Shadow Texture creation failed"));
            return;
        }

        // SRV
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = 1;

        hr = Device->CreateShaderResourceView(ShadowTexture.Get(), &srvDesc, &ShadowSRV);
        if (FAILED(hr))
        {
            assert(TEXT("Shadow SRV creation failed"));
            return;
        }

        // DSV 6 faces
        for (UINT face = 0; face < 6; ++face)
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice = 0;
            dsvDesc.Texture2DArray.FirstArraySlice = face;
            dsvDesc.Texture2DArray.ArraySize = 1;
            ComPtr<ID3D11DepthStencilView> dsv;
            hr = Device->CreateDepthStencilView(ShadowTexture.Get(), &dsvDesc, &dsv);
            if (FAILED(hr))
            {
                assert(TEXT("Shadow DSV creation failed"));
                return;
            }
            ShadowDSVs.Add(dsv);
            // Viewport 설정
            D3D11_VIEWPORT viewport = {};
            viewport.TopLeftX = 0;
            viewport.TopLeftY = 0;
            viewport.Width = static_cast<FLOAT>(ShadowResolution);
            viewport.Height = static_cast<FLOAT>(ShadowResolution);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            Viewports.Add(viewport);
        }
        break;
    }
    }
}

FShadowResource::~FShadowResource()
{
    // remove this from the factory
    TArray<FShadowResource*>& ShadowResourcesArray = FShadowResourceFactory::ShadowResources[LightType];
    ShadowResourcesArray.RemoveSingle(this);
}

size_t FShadowResource::GetEsimatedMemoryUsageInBytes() const
{
    if (!ShadowTexture)
        return 0;

    D3D11_TEXTURE2D_DESC Desc;
    ShadowTexture->GetDesc(&Desc);

    size_t PixelSizeInBytes = 0;
    switch (Desc.Format)
    {
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        PixelSizeInBytes = 4;
        break;
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
        PixelSizeInBytes = 2;
        break;
    default:
        return 0;
    }

    size_t SizePerTex = static_cast<size_t>(Desc.Width) *
        static_cast<size_t>(Desc.Height) *
        PixelSizeInBytes;

    // 멀티샘플링 고려
    size_t SampleCount = FMath::Max((UINT)1, Desc.SampleDesc.Count);
    SizePerTex *= SampleCount;

    // 배열/밉맵 고려
    size_t Total = SizePerTex * Desc.ArraySize;
    if (Desc.MipLevels > 1)
    {
        // 밉맵 체인은 상위 레벨의 1/4씩 추가됨 (대략적인 추정)
        Total += Total / 3;
    }

    // 뷰 객체 메모리 추정 (대략 1KB per view)
    const size_t ViewOverhead = 1024;
    size_t ViewCount = 0;
    if (ShadowSRV) ViewCount++;
    ViewCount += ShadowDSVs.Num();
    Total += ViewCount * ViewOverhead;

    return Total;
}

FShadowResource* FShadowResourceFactory::CreateShadowResource(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution)
{
    FShadowResource* shadowResource = new FShadowResource(Device, LightType, ShadowResolution);
    if (shadowResource)
    {
        if (ShadowResources.Contains(shadowResource->LightType))
        {
            ShadowResources[shadowResource->LightType].Add(shadowResource);
        }
        else
        {
            ShadowResources.Add(shadowResource->LightType, TArray<FShadowResource*>());
            ShadowResources[shadowResource->LightType].Add(shadowResource);
        }
    }
    else
    {
        // Handle error
        assert(TEXT("Failed to create shadow resource"));
    }
    return shadowResource;
};
FShadowMemoryUsageInfo FShadowResourceFactory::GetShadowMemoryUsageInfo()
{
    FShadowMemoryUsageInfo memoryUsageInfo;
    for (const auto& pair : ShadowResources)
    {
        ELightType lightType = pair.Key;
        const TArray<FShadowResource*>& shadowResources = pair.Value;
        size_t totalMemory = 0;
        size_t count = 0;
        for (const FShadowResource* shadowResource : shadowResources)
        {
            totalMemory += shadowResource->GetEsimatedMemoryUsageInBytes();
            count++;
        }
        memoryUsageInfo.TotalMemoryUsage += totalMemory;
        memoryUsageInfo.MemoryUsageByLightType.Add(lightType, totalMemory);
        memoryUsageInfo.LightCountByLightType.Add(lightType, count);
    }
    return memoryUsageInfo;
}
