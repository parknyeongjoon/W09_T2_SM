#pragma once  
#include "Define.h"  
#include "Container/Map.h"

class FShadowAtlas;

struct FShadowResource  
{  
   static UINT ShadowResolution;

   FShadowAtlas* ParentAtlas = nullptr;
   UINT AtlasSliceIndex = 0;

   //Atlas 내 위치 정보
   D3D11_VIEWPORT SliceViewport;

   // Face의 개수. Directional/Spot Light는 1개, Point Light는 6개..  
   int NumFaces = 1;  

   ELightType LightType;  

   FShadowResource() = default;
   FShadowResource(FShadowAtlas* Atlas, ELightType InLightType, UINT SliceIndex);
   ~FShadowResource();

   size_t GetEsimatedMemoryUsageInBytes() const;
   ELightType GetLightType() const { return LightType; }

   ID3D11ShaderResourceView* GetSRV() const;
   ID3D11Texture2D* GetTexture() const;
   ID3D11DepthStencilView* GetDSV(int faceIndex = 0) const;
   D3D11_VIEWPORT GetViewport(int faceIndex = 0) const;
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
    static inline TArray<FShadowAtlas*> ShadowAtlases;
    static inline TMap<ELightType, TArray<FShadowResource*>> ShadowResources;
    static FShadowResource* CreateShadowResource(ID3D11Device* Device, ELightType LightType);
    static FShadowMemoryUsageInfo GetShadowMemoryUsageInfo();

private:
    static FShadowAtlas* FindOrCreateAtlas(ID3D11Device* Device, ELightType LightType, bool bForceCreateNew = false);
};

