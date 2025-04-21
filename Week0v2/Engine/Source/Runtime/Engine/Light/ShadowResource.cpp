#include "ShadowResource.h"
#include "Math/MathUtility.h"
#include "ShadowResourceFactory.h"

FShadowResourceBase::FShadowResourceBase(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution)
    :LightType(LightType)
    , ShadowResolution(ShadowResolution)
{

}

FShadowResourceBase::~FShadowResourceBase()
{
    // remove this from the factory
    TArray<FShadowResourceBase*>& ShadowResourcesArray = FShadowResourceFactory::ShadowResources[LightType];
    ShadowResourcesArray.RemoveSingle(this);
}

size_t FShadowResourceBase::GetEstimatedMemoryUsageInBytes() const
{

    return 0;
}

FShadowResource2D::FShadowResource2D(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution)
    : FShadowResourceBase(Device, LightType, ShadowResolution)
{
    // Texture2D는 전역 텍스쳐
    // SRV도 전역 SRV
    // DSV와 뷰포트는 각각
    NumFaces = 1;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    ShadowTexture = FShadowResourceFactory::GetShadowAtlasTexture();

    ComPtr<ID3D11DepthStencilView> dsv;

    HRESULT hr = Device->CreateDepthStencilView(ShadowTexture, &dsvDesc, &dsv);
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

size_t FShadowResource2D::GetEstimatedMemoryUsageInBytes() const
{
    return size_t();
}

ID3D11ShaderResourceView* FShadowResource2D::GetSRV() const
{
    return FShadowResourceFactory::GetShadowAtlasSRV();
}

FShadowResourceCube::FShadowResourceCube(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution)
{
    NumFaces = 6;
    throw std::runtime_error("FShadowResourceCube is not implemented yet");
}

size_t FShadowResourceCube::GetEstimatedMemoryUsageInBytes() const
{
    // !TODO
    return size_t();
}

FShadowResourceCubeArray::FShadowResourceCubeArray(ID3D11Device* Device, UINT ShadowResolution, UINT MaxCubeCount)
    : FShadowResourceBase(Device, ELightType::PointLight, ShadowResolution)
{

    // 각 큐브맵 면에 대한 DSV(Depth Stencil View) 생성
    ShadowDSVs.Reserve(6 * MaxCubeCount);
    Viewports.Reserve(6 * MaxCubeCount);

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    ZeroMemory(&dsvDesc, sizeof(dsvDesc));
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    dsvDesc.Texture2DArray.ArraySize = 1;
    dsvDesc.Texture2DArray.FirstArraySlice = 0;
    dsvDesc.Texture2DArray.MipSlice = 0;

    ShadowCubeArrayTexture = FShadowResourceFactory::GetShadowCubeArrayTexture();
    ShadowCubeArraySRV = FShadowResourceFactory::GetShadowCubeArraySRV();

    for (int cubeIdx = 0; cubeIdx < MaxCubeCount; ++cubeIdx) {
        for (int faceIdx = 0; faceIdx < 6; ++faceIdx) {
            // DSV 생성
            dsvDesc.Texture2DArray.FirstArraySlice = cubeIdx * 6 + faceIdx;
            ComPtr<ID3D11DepthStencilView> dsv;
            HRESULT hr = Device->CreateDepthStencilView(
                ShadowCubeArrayTexture,
                &dsvDesc,
                &dsv);

            if (FAILED(hr)) {
                throw std::runtime_error("Failed to create DSV for cube map face");
            }
            ShadowDSVs.Add(dsv);

            // 뷰포트 설정
            D3D11_VIEWPORT vp;
            vp.Width = static_cast<float>(ShadowResolution);
            vp.Height = static_cast<float>(ShadowResolution);
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            vp.TopLeftX = 0.0f;
            vp.TopLeftY = 0.0f;
            Viewports.Add(vp);
        }
    }
}

size_t FShadowResourceCubeArray::GetEstimatedMemoryUsageInBytes() const
{
    // !TODO
    return size_t();
}

ID3D11ShaderResourceView* FShadowResourceCubeArray::GetSRV() const
{
    return FShadowResourceFactory::GetShadowCubeArraySRV();
}
