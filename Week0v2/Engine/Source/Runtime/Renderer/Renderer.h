#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define _TCHAR_DEFINED
#include "EngineBaseTypes.h"
#include "Define.h"
#include "RenderResourceManager.h"
#include "RenderPass/DebugDepthRenderPass.h"
#include "RenderPass/FogRenderPass.h"
#include "RenderPass/ShadowRenderPass.h"
#include "RenderPass/SkeletalMeshRenderPass.h"

class FLetterBoxRenderPass;
struct FLetterBoxConstants;
class FBlurRenderPass;
class FFinalRenderPass;
class FFadeRenderPass;
class FComputeTileLightCulling;
class FEditorIconRenderPass;
class FGizmoRenderPass;
class FLineBatchRenderPass;
class FStaticMeshRenderPass;
class FEditorViewportClient;
class UWorld;

class FRenderer 
{
private:
    void CreateVertexPixelShader(const FString& InPrefix, D3D_SHADER_MACRO* pDefines);
    //void CreateComputeShader();
    //void CreateStaticMeshShader();
    //void CreateTextureShader();
    //void CreateLineBatchShader();
    //void CreateFogShader();
    //void CreateDebugDepthShader();

public:
    FGraphicsDevice* Graphics;
    ID3D11SamplerState* GetSamplerState(const ESamplerType InType) const { return RenderResourceManager->GetSamplerState(InType); }
    ID3D11RasterizerState* GetRasterizerState(const ERasterizerState InState) const { return RenderResourceManager->GetRasterizerState(InState); }
    ID3D11BlendState* GetBlendState(const EBlendState InState) const { return RenderResourceManager->GetBlendState(InState); }
    ID3D11DepthStencilState* GetDepthStencilState(const EDepthStencilState InState) const { return RenderResourceManager->GetDepthStencilState(InState); }

    ID3D11RasterizerState* GetCurrentRasterizerState() const {  return RenderResourceManager->GetRasterizerState(CurrentRasterizerState); }
    void SetCurrentRasterizerState(const ERasterizerState InState) { CurrentRasterizerState = InState; }

    std::shared_ptr<FVBIBTopologyMapping> GetVBIBTopologyMapping(const FName InName) {return VBIBTopologyMappings[InName];}

    bool bIsLit = true;
    bool bIsNormal = false;
    bool bIsDepth = false;
public:
    void Initialize(FGraphicsDevice* graphics);

    static D3D_SHADER_MACRO GouradDefines[];
    static D3D_SHADER_MACRO LambertDefines[];
    static D3D_SHADER_MACRO EditorGizmoDefines[];
    static D3D_SHADER_MACRO EditorIconDefines[];
    
    //Release
    void Release();
    
    void SetViewMode(EViewModeIndex evi);
    EViewModeIndex GetCurrentViewMode() const { return CurrentViewMode; }

    void SetShadowFilterMode(EShadowFilterMode FilterMode) { CurrentShadowFilterMode = FilterMode; }
    EShadowFilterMode GetShadowFilterMode() { return CurrentShadowFilterMode; }

public:
    //Render Pass Demo
    
    void AddRenderObjectsToRenderPass() const;
    void Render(const std::shared_ptr<FEditorViewportClient>& ActiveViewport);
    void EndRender() const;
    void ClearRenderObjects() const;

public:
    void PrepareShader(FName InShaderName);
    void BindConstantBuffers(FName InShaderName);
    
public:
    void CreateMappedCB(TMap<FShaderConstantKey, uint32>& ShaderStageToCB, const TArray<FConstantBufferInfo>& CBArray, EShaderStage Stage) const;
    
    void MappingVSPSInputLayout(FName InShaderProgramName, FName VSName, FName PSName, FName InInputLayoutName);
    void MappingVSPSCBSlot(FName InShaderName, const TMap<FShaderConstantKey, uint32>& MappedConstants);
    void MappingVBTopology(FName InObjectName, FName InVBName, uint32 InStride, uint32 InNumVertices, D3D11_PRIMITIVE_TOPOLOGY InTopology= D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    void MappingIB(FName InObjectName, FName InIBName, uint32 InNumIndices);
private: 
    TMap<FName, std::shared_ptr<FShaderProgram>> ShaderPrograms;
    TMap<FName, TMap<FShaderConstantKey, uint32>> ShaderConstantNameAndSlots;
    TMap<FName, std::shared_ptr<FVBIBTopologyMapping>> VBIBTopologyMappings;
public:
    FRenderResourceManager* GetResourceManager() const { return RenderResourceManager; }
public:
    size_t GetAtlasMemoryUsage(ELightType lightType) const;
private:
    FRenderResourceManager* RenderResourceManager = nullptr;

    std::shared_ptr<FStaticMeshRenderPass> GoroudRenderPass;
    std::shared_ptr<FStaticMeshRenderPass> LambertRenderPass;
    std::shared_ptr<FStaticMeshRenderPass> PhongRenderPass;
    std::shared_ptr<FLineBatchRenderPass> LineBatchRenderPass;
    std::shared_ptr<FGizmoRenderPass> GizmoRenderPass;
    //std::shared_ptr<FComputeTileLightCulling> ComputeTileLightCulling;
    
    std::shared_ptr<FDebugDepthRenderPass> DebugDepthRenderPass;
    std::shared_ptr<FEditorIconRenderPass> EditorIconRenderPass;
    std::shared_ptr<FFogRenderPass> FogRenderPass;
    std::shared_ptr<FShadowRenderPass> ShadowRenderPass;
    std::shared_ptr<FFadeRenderPass> FadeRenderPass;
    std::shared_ptr<FLetterBoxRenderPass> LetterBoxRenderPass;
    std::shared_ptr<FBlurRenderPass> BlurRenderPass;
    std::shared_ptr<FFinalRenderPass> FinalRenderPass;
    std::shared_ptr<FSkeletalMeshRenderPass> SkeletalRenderPass;

    ERasterizerState CurrentRasterizerState = ERasterizerState::SolidBack;
    EViewModeIndex CurrentViewMode = VMI_Lit_Goroud;
    EShadowFilterMode CurrentShadowFilterMode = EShadowFilterMode::PCF;
};

