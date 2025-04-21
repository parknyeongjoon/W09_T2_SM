#define _TCHAR_DEFINED

#include <wrl/client.h> 
#include <queue>
#include "Define.h"

using Microsoft::WRL::ComPtr;

class FShadowAtlas

{
private:
    ComPtr<ID3D11Texture2D> AtlasTexture;
    ComPtr<ID3D11ShaderResourceView> AtlasSRV;
    ComPtr<ID3D11DepthStencilView> AtlasDSV;
    D3D11_VIEWPORT AtlasViewport;

    std::queue<UINT> FreeSlices;

    UINT Resolution;
    UINT CurrentAllocation = 0;
    UINT MaxShadows;

public:
    FShadowAtlas(ID3D11Device* Device, UINT InResolution, UINT InMaxShadows);
    ~FShadowAtlas();

    bool AllocateShadowSlice(UINT& OutSlideIndex);
    void FreeShadowSlice(UINT SlideIndex);

    ID3D11ShaderResourceView* GetSRV() const;
    ID3D11DepthStencilView* GetDSV() const;
    ID3D11Texture2D* GetTexture() const;
    D3D11_VIEWPORT GetViewport() const;
    UINT GetResolution() const;
    UINT GetMaxShadows() const;
    UINT GetCurrentAllocation() const;

    size_t GetMemoryUsage() const;
};