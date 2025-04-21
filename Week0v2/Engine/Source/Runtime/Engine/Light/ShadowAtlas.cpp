#include "ShadowAtlas.h"
#include "ShadowResource.h"


FShadowAtlas::FShadowAtlas(ID3D11Device* Device, UINT InResolution, UINT InMaxShadows)
    : Resolution(InResolution)
    , MaxShadows(InMaxShadows)
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    ZeroMemory(&texDesc, sizeof(texDesc));
    texDesc.Width = Resolution;
    texDesc.Height = Resolution * MaxShadows; // Vertical stacking
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = Device->CreateTexture2D(&texDesc, nullptr, &AtlasTexture);
    if (FAILED(hr))
    {
        assert(TEXT("Shadow Atlas Texture creation failed"));
        return;
    }

    // srv
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hr = Device->CreateShaderResourceView(AtlasTexture.Get(), &srvDesc, &AtlasSRV);
    if (FAILED(hr))
    {
        assert(TEXT("Shadow Atlas SRV creation failed"));
        return;
    }

    // dsv
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = 0;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = Device->CreateDepthStencilView(AtlasTexture.Get(), &dsvDesc, &AtlasDSV);
    if (FAILED(hr))
    {
        assert(TEXT("Shadow Atlas DSV creation failed"));
        return;
    }

    AtlasViewport.Width = static_cast<FLOAT>(Resolution);
    AtlasViewport.Height = static_cast<FLOAT>(Resolution);
    AtlasViewport.MinDepth = 0.0f;
    AtlasViewport.MaxDepth = 1.0f;
    AtlasViewport.TopLeftX = 0;
    AtlasViewport.TopLeftY = 0;

    // init queue
    for (UINT i = 0; i < MaxShadows; ++i)
    {
        FreeSlices.push(i);
    }
}

FShadowAtlas::~FShadowAtlas()
{
    // 자원들은 Comptr이 알아서 해제
    // Factory에서 이 아틀라스 제거
    FShadowResourceFactory::ShadowAtlases.Remove(this);
}

bool FShadowAtlas::AllocateShadowSlice(UINT& OutSlideIndex)
{
    if (FreeSlices.empty())
        return false;

    OutSlideIndex = FreeSlices.front();
    FreeSlices.pop();

    CurrentAllocation++;
    return true;
}

void FShadowAtlas::FreeShadowSlice(UINT SlideIndex)
{
    if (SlideIndex < MaxShadows)
    {
        FreeSlices.push(SlideIndex);
        CurrentAllocation--;
    }

    // 아틀라스에 할당된 슬라이스가 없으면 Factory에서 삭제 및 이 아틀라스 제거
    if (CurrentAllocation == 0)
    {
        delete this;
    }
}

ID3D11DepthStencilView* FShadowAtlas::GetDSV() const
{
    return AtlasDSV.Get();
}

ID3D11Texture2D* FShadowAtlas::GetTexture() const
{
    return AtlasTexture.Get();
}

D3D11_VIEWPORT FShadowAtlas::GetViewport() const
{
    return D3D11_VIEWPORT();
}

UINT FShadowAtlas::GetResolution() const
{
    return Resolution;
}

UINT FShadowAtlas::GetMaxShadows() const
{
    return MaxShadows;
}

ID3D11ShaderResourceView* FShadowAtlas::GetSRV() const
{
    return AtlasSRV.Get();
}

UINT FShadowAtlas::GetCurrentAllocation() const
{
    return CurrentAllocation;
}

size_t FShadowAtlas::GetMemoryUsage() const
{
    return Resolution * Resolution * 4 * MaxShadows; // 4 bytes per pixel
}