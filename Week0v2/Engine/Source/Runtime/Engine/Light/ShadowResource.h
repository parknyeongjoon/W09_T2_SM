#pragma once  
#include "Define.h"  
#include "Container/Map.h"
#define _TCHAR_DEFINED
#include <wrl/client.h> 

using Microsoft::WRL::ComPtr;

struct FShadowResource  
{  
   ComPtr<ID3D11ShaderResourceView> ShadowSRV; // Example usage of ComPtr  
   TArray<ComPtr<ID3D11DepthStencilView>> ShadowDSVs;  
   ComPtr<ID3D11Texture2D> ShadowTexture;  
   TArray<D3D11_VIEWPORT> Viewports;  

   UINT ShadowResolution;

   // Face의 개수. Directional/Spot Light는 1개, Point Light는 6개..  
   int NumFaces = 1;  

   ELightType LightType;  

   FShadowResource() = default;
   FShadowResource(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution);
   ~FShadowResource();

   size_t GetEsimatedMemoryUsageInBytes() const;
   ELightType GetLightType() const { return LightType; }
   ID3D11ShaderResourceView* GetSRV() const { return ShadowSRV.Get(); }
   ID3D11Texture2D* GetTexture() const { return ShadowTexture.Get(); }
   ID3D11DepthStencilView* GetDSV(int faceIndex = 0) const
   {
       if (faceIndex < 0 || faceIndex >= ShadowDSVs.Num())
           return nullptr;
       return ShadowDSVs[faceIndex].Get();
   }
   D3D11_VIEWPORT GetViewport(int faceIndex = 0) const
   {
       if (faceIndex < 0 || faceIndex >= Viewports.Num())
           return {};
       return Viewports[faceIndex];
   }
};

struct FShadowMemoryUsageInfo
{
    size_t TotalMemoryUsage = 0;
    TMap<ELightType, size_t> MemoryUsageByLightType;
    TMap<ELightType, size_t> LightCountByLightType;
};

class FShadowResourceFactory
{
public:
    static inline TMap<ELightType, TArray<FShadowResource*>> ShadowResources;
    static FShadowResource* CreateShadowResource(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution);
    static FShadowMemoryUsageInfo GetShadowMemoryUsageInfo();
};

