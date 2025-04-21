#pragma once  
#include "Define.h"  
#define _TCHAR_DEFINED
#include <wrl/client.h> 

using Microsoft::WRL::ComPtr;
// 리소스 타입 구분
enum class EShadowResourceType
{
    Texture2D,      // Directional/Spot Light용
    TextureCube,    // Point Light용 단일 큐브맵
    TextureCubeArray // Point Light용 큐브맵 배열
};

struct FShadowResourceBase  
{  
   UINT ShadowResolution;

   // Face의 개수. Directional/Spot Light는 1개, Point Light는 6개..  
   int NumFaces = 1;  

   ELightType LightType;  

   FShadowResourceBase() = default;
   FShadowResourceBase(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution);

   ELightType GetLightType() const { return LightType; }

   virtual ~FShadowResourceBase();
   virtual size_t GetEstimatedMemoryUsageInBytes() const = 0;
   virtual EShadowResourceType GetResourceType() const = 0;
};

// Directional, Spot Light용 2D 텍스처
struct FShadowResource2D : public FShadowResourceBase
{
    ID3D11Texture2D* ShadowTexture = nullptr;
    ID3D11ShaderResourceView* ShadowSRV = nullptr; 

    FShadowResource2D(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution);
    size_t GetEstimatedMemoryUsageInBytes() const override;
    EShadowResourceType GetResourceType() const override { return EShadowResourceType::Texture2D; }
};

// Point Light용 큐브맵
struct FShadowResourceCube : public FShadowResourceBase
{
    ComPtr<ID3D11Texture2D> ShadowCubeTexture;
    ComPtr<ID3D11ShaderResourceView> ShadowSRV; // 얘는 아틀라스와는 독립적임. SRV 필요

    FShadowResourceCube(ID3D11Device* Device, ELightType LightType, UINT ShadowResolution);
    size_t GetEstimatedMemoryUsageInBytes() const override;
    EShadowResourceType GetResourceType() const override { return EShadowResourceType::TextureCube; }
};

// Point Light용 큐브맵 배열
struct FShadowResourceCubeArray : public FShadowResourceBase
{
    ID3D11Texture2D* ShadowCubeArrayTexture = nullptr;
    ID3D11ShaderResourceView* ShadowCubeArraySRV = nullptr; 
    int ArraySize;

    FShadowResourceCubeArray(ID3D11Device* Device, UINT ShadowResolution, UINT MaxCubeCount);
    size_t GetEstimatedMemoryUsageInBytes() const override;
    EShadowResourceType GetResourceType() const override { return EShadowResourceType::TextureCubeArray; }
};
