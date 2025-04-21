#pragma once
#include "Define.h"
#include "Container/Map.h"
#define _TCHAR_DEFINED
#include <wrl/client.h> 

using Microsoft::WRL::ComPtr;

class FShadowResourceBase;

struct FShadowMemoryUsageInfo
{
    size_t TotalMemoryUsage = 0;
    TMap<ELightType, size_t> MemoryUsageByLightType;
    TMap<ELightType, size_t> LightCountByLightType;
};

class FShadowResourceFactory
{
private:    
    // 일단 지금은 아틀라스1장
    static inline ComPtr<ID3D11Texture2D> ShadowAtlasTexture;
    static inline ComPtr<ID3D11Texture2D> ShadowCubeArrayTexture;
    static inline ComPtr<ID3D11Texture2D> DirectionalShadowTexture;
public:
    static ID3D11Texture2D* GetShadowAtlasTexture() { return ShadowAtlasTexture.Get(); }
    static ID3D11Texture2D* GetShadowCubeArrayTexture() { return ShadowCubeArrayTexture.Get(); }
    static ID3D11Texture2D* GetDirectionalShadowTexture() { return DirectionalShadowTexture.Get(); }
public:
    static inline TMap<ELightType, TArray<FShadowResourceBase*>> ShadowResources;

    // 하나의 텍스쳐에 대한 SRV
    static inline ComPtr<ID3D11ShaderResourceView> DirectionalShadowSRV;
    static inline ComPtr<ID3D11ShaderResourceView> ShadowAtlasSRV;
    static inline ComPtr<ID3D11ShaderResourceView> ShadowCubeArraySRV;

    static ID3D11ShaderResourceView* GetShadowAtlasSRV() { return ShadowAtlasSRV.Get(); }
    static ID3D11ShaderResourceView* GetShadowCubeArraySRV() { return ShadowCubeArraySRV.Get(); }
    static ID3D11ShaderResourceView* GetDirectionalShadowSRV() { return DirectionalShadowSRV.Get(); }

    static inline UINT AtlasSize = 4096;
    static inline UINT CubeArraySize = 2048;
    static inline UINT MaxCubeCount = 16;

    static inline bool bInitialized = false;
public:
    static void Initialize(ID3D11Device* Device);
    static void ReleaseAllResources();
    static FShadowResourceBase* CreateShadowResource(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution);
    static FShadowMemoryUsageInfo GetShadowMemoryUsageInfo();

private:
    static FShadowResourceBase* CreateDirectionalSpotResource(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution);
    static FShadowResourceBase* CreatePointLightResource(ID3D11Device* Device, UINT ShadowResolution);
};