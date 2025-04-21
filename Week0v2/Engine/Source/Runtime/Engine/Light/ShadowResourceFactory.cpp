#include "ShadowResourceFactory.h"
#include "ShadowResource.h"

void FShadowResourceFactory::Initialize(ID3D11Device* Device)
{
    if (bInitialized)
        return;

    D3D11_TEXTURE2D_DESC atlasDesc;
    ZeroMemory(&atlasDesc, sizeof(atlasDesc));
    atlasDesc.Width = AtlasSize;
    atlasDesc.Height = AtlasSize;
    atlasDesc.MipLevels = 1;
    atlasDesc.ArraySize = 1;
    atlasDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    atlasDesc.SampleDesc.Count = 1;
    atlasDesc.Usage = D3D11_USAGE_DEFAULT;
    atlasDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    Device->CreateTexture2D(&atlasDesc, nullptr, &ShadowAtlasTexture);

    // Point Light용 큐브맵 배열 생성
    D3D11_TEXTURE2D_DESC cubeArrayDesc;
    ZeroMemory(&cubeArrayDesc, sizeof(cubeArrayDesc));
    cubeArrayDesc.Width = CubeArraySize;
    cubeArrayDesc.Height = CubeArraySize;
    cubeArrayDesc.MipLevels = 1;
    cubeArrayDesc.ArraySize = MaxCubeCount; // 6 faces per cube
    cubeArrayDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    cubeArrayDesc.SampleDesc.Count = 1;
    cubeArrayDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    cubeArrayDesc.Usage = D3D11_USAGE_DEFAULT;
    cubeArrayDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    Device->CreateTexture2D(&cubeArrayDesc, nullptr, &ShadowCubeArrayTexture);

    

    // Create Shader Resource View for the atlas
    D3D11_SHADER_RESOURCE_VIEW_DESC atlasSRVDesc;
    ZeroMemory(&atlasSRVDesc, sizeof(atlasSRVDesc));
    atlasSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
    atlasSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    atlasSRVDesc.Texture2D.MipLevels = 1;
    atlasSRVDesc.Texture2D.MostDetailedMip = 0;
    Device->CreateShaderResourceView(ShadowAtlasTexture.Get(), &atlasSRVDesc, &ShadowAtlasSRV);

    // Create Shader Resource View for the cube array
    D3D11_SHADER_RESOURCE_VIEW_DESC cubeArraySRVDesc;
    ZeroMemory(&cubeArraySRVDesc, sizeof(cubeArraySRVDesc));
    cubeArraySRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
    cubeArraySRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
    cubeArraySRVDesc.TextureCubeArray.MipLevels = 1;
    cubeArraySRVDesc.TextureCubeArray.MostDetailedMip = 0;
    cubeArraySRVDesc.TextureCubeArray.NumCubes = MaxCubeCount;
    Device->CreateShaderResourceView(ShadowCubeArrayTexture.Get(), &cubeArraySRVDesc, &ShadowCubeArraySRV);

    bInitialized = true;
}

void FShadowResourceFactory::ReleaseAllResources()
{
}

FShadowResourceBase* FShadowResourceFactory::CreateShadowResource(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution)
{
    switch (LightType)
    {
    case ELightType::DirectionalLight:
    case ELightType::SpotLight:
        return CreateDirectionalSpotResource(Device, LightType, ShadowResolution);
    case ELightType::PointLight:
        return CreatePointLightResource(Device, ShadowResolution);
    default:
        assert(TEXT("Invalid light type for shadow resource creation"));
        return nullptr;
    }
};

FShadowMemoryUsageInfo FShadowResourceFactory::GetShadowMemoryUsageInfo()
{
    FShadowMemoryUsageInfo memoryUsageInfo;

    for (auto& pair : ShadowResources)
    {
        for (auto* Resource : pair.Value)
        {
            size_t Usage = Resource->GetEstimatedMemoryUsageInBytes();
            memoryUsageInfo.TotalMemoryUsage += Usage;
            memoryUsageInfo.MemoryUsageByLightType.FindOrAdd(Resource->GetLightType()) += Usage;
            memoryUsageInfo.LightCountByLightType.FindOrAdd(Resource->GetLightType())++;
        }
    }

    return memoryUsageInfo;
}

FShadowResourceBase* FShadowResourceFactory::CreateDirectionalSpotResource(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution)
{
    auto Resource = new FShadowResource2D(Device, LightType, ShadowResolution);
    if (Resource)
    {
        ShadowResources.FindOrAdd(LightType).Add(Resource);
    }
    return Resource;
}

FShadowResourceBase* FShadowResourceFactory::CreatePointLightResource(ID3D11Device* Device, UINT ShadowResolution)
{
    auto Resource = new FShadowResourceCubeArray(Device, ShadowResolution, MaxCubeCount);
    if (Resource)
    {
        ShadowResources.FindOrAdd(ELightType::PointLight).Add(Resource);
    }
    return Resource;
}
