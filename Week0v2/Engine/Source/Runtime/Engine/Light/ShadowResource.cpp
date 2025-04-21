#include "ShadowResource.h"
#include "Math/MathUtility.h"
#include "ShadowAtlas.h"


UINT FShadowResource::ShadowResolution = 1024; // Default shadow resolution

FShadowResource::FShadowResource(FShadowAtlas* Atlas, ELightType InLightType, UINT SliceIndex)
    : ParentAtlas(Atlas)
    , LightType(InLightType)
    , AtlasSliceIndex(SliceIndex)
{
    SliceViewport.Width = static_cast<float>(ParentAtlas->GetViewport().Height);
    SliceViewport.Height = static_cast<float>(ParentAtlas->GetViewport().Width);
    SliceViewport.TopLeftX = 0;
    SliceViewport.TopLeftY = static_cast<float>(SliceIndex) * SliceViewport.Height;
    SliceViewport.MinDepth = 0.0f;
    SliceViewport.MaxDepth = 1.0f;

    NumFaces = LightType == ELightType::PointLight ? 6 : 1;
}

FShadowResource::~FShadowResource()
{
    // 아틀라스에서 이 슬라이스 해제
    if (ParentAtlas)
    {
        ParentAtlas->FreeShadowSlice(AtlasSliceIndex);
    }
    // remove this from the factory
    TArray<FShadowResource*>& ShadowResourcesArray = FShadowResourceFactory::ShadowResources[LightType];
    ShadowResourcesArray.RemoveSingle(this);
}

size_t FShadowResource::GetEsimatedMemoryUsageInBytes() const
{
    if (ParentAtlas)
    {
        return ParentAtlas->GetMemoryUsage() / ParentAtlas->GetMaxShadows();
    }
    return 0;
}

ID3D11ShaderResourceView* FShadowResource::GetSRV() const
{
    return ParentAtlas ? ParentAtlas->GetSRV() : nullptr;
}

ID3D11Texture2D* FShadowResource::GetTexture() const
{
    return ParentAtlas ? ParentAtlas->GetTexture() : nullptr;
}

ID3D11DepthStencilView* FShadowResource::GetDSV(int faceIndex) const
{
    if (!ParentAtlas || faceIndex < 0 || faceIndex >= NumFaces)
        return nullptr;

    return ParentAtlas->GetDSV();
}

D3D11_VIEWPORT FShadowResource::GetViewport(int faceIndex) const
{
    if (faceIndex < 0 || faceIndex >= NumFaces)
        return {};

    D3D11_VIEWPORT vp = SliceViewport;

    if (LightType == ELightType::PointLight)
    {
        // Adjust viewport for cubemap face (simplified - may need more complex logic)
        vp.TopLeftY += faceIndex * (vp.Height / 6);
        vp.Height /= 6;
    }

    return vp;
}

FShadowResource* FShadowResourceFactory::CreateShadowResource(ID3D11Device* Device, ELightType LightType)
{
    FShadowAtlas* atlas = FindOrCreateAtlas(Device, LightType);
    if (!atlas)
    {
        return nullptr;
    }

    UINT SliceIndex = 0;
    if (!atlas->AllocateShadowSlice(SliceIndex))
    {
        // atlas is full, create a new one
        atlas = FindOrCreateAtlas(Device, LightType, true);
        if (!atlas)
        {
            return nullptr;
        }
    }

    FShadowResource* NewResource = new FShadowResource(atlas, LightType, SliceIndex);
    ShadowResources.FindOrAdd(LightType).Add(NewResource);

    return NewResource;
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

FShadowAtlas* FShadowResourceFactory::FindOrCreateAtlas(ID3D11Device* Device, ELightType LightType, bool bForceCreateNew)
{
    UINT ShadowResolution = FShadowResource::ShadowResolution;
    UINT ShadowsPerAtlas = 4;

    if (!bForceCreateNew)
    {
        for (auto& Atlas : ShadowAtlases)
        {
            if (Atlas->GetCurrentAllocation() < Atlas->GetMaxShadows())
            {
                return Atlas;
            }
        }
    }

    // Create a new atlas if no existing one can be used
    FShadowAtlas* NewAtlas = new FShadowAtlas(Device, ShadowResolution, ShadowsPerAtlas);
    ShadowAtlases.Add(NewAtlas);
    return NewAtlas;
}
