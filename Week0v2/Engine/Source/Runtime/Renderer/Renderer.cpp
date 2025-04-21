#include "Renderer.h"
#include <d3dcompiler.h>

#include "VBIBTopologyMapping.h"
#include "ComputeShader/ComputeTileLightCulling.h"
#include "Engine/World.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Launch/EditorEngine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "PropertyEditor/ShowFlags.h"
#include "UObject/UObjectIterator.h"
#include "D3D11RHI/FShaderProgram.h"
#include "RenderPass/EditorIconRenderPass.h"
#include "RenderPass/GizmoRenderPass.h"
#include "RenderPass/LineBatchRenderPass.h"
#include "RenderPass/StaticMeshRenderPass.h"
#include "Light/ShadowResourceFactory.h"

D3D_SHADER_MACRO FRenderer::GouradDefines[] =
{
    {"LIGHTING_MODEL_GOURAUD", "1"},
    {nullptr, nullptr}
};

D3D_SHADER_MACRO FRenderer::LambertDefines[] = 
{
    {"LIGHTING_MODEL_LAMBERT", "1"},
    {nullptr, nullptr}
};

D3D_SHADER_MACRO FRenderer::EditorGizmoDefines[] = 
{
    {"RENDER_GIZMO", "1"},
    {nullptr, nullptr}
};

D3D_SHADER_MACRO FRenderer::EditorIconDefines[] = 
{
    {"RENDER_ICON", "1"},
    {nullptr, nullptr}
};

void FRenderer::Initialize(FGraphicsDevice* graphics)
{
    Graphics = graphics;
    RenderResourceManager = new FRenderResourceManager(graphics);
    RenderResourceManager->Initialize();

    CreateComputeShader();
    
    D3D_SHADER_MACRO defines[] = 
    {
        {"LIGHTING_MODEL_GOURAUD", "1"},
        {nullptr, nullptr}
    };
    //SetViewMode(VMI_Lit_Phong);
    
    CreateVertexPixelShader(TEXT("UberLit"), GouradDefines);
    FString GouradShaderName = TEXT("UberLit");
    GouradShaderName += GouradDefines->Name;
    GoroudRenderPass = std::make_shared<FStaticMeshRenderPass>(GouradShaderName);

    CreateVertexPixelShader(TEXT("UberLit"), LambertDefines);
    FString LamberShaderName = TEXT("UberLit");
    LamberShaderName += LambertDefines->Name;
    LambertRenderPass = std::make_shared<FStaticMeshRenderPass>(LamberShaderName);
    
    CreateVertexPixelShader(TEXT("UberLit"), nullptr);
    FString PhongShaderName = TEXT("UberLit");
    PhongRenderPass = std::make_shared<FStaticMeshRenderPass>(PhongShaderName);
    
    CreateVertexPixelShader(TEXT("Line"), nullptr);
    LineBatchRenderPass = std::make_shared<FLineBatchRenderPass>(TEXT("Line"));

    CreateVertexPixelShader(TEXT("DebugDepth"), nullptr);
    DebugDepthRenderPass = std::make_shared<FDebugDepthRenderPass>(TEXT("DebugDepth"));
    
    FString GizmoShaderName = TEXT("Editor");
    GizmoShaderName += EditorGizmoDefines->Name;
    CreateVertexPixelShader(TEXT("Editor"), EditorGizmoDefines);
    GizmoRenderPass = std::make_shared<FGizmoRenderPass>(GizmoShaderName);
    
    FString IconShaderName = TEXT("Editor");
    IconShaderName += EditorIconDefines->Name;
    CreateVertexPixelShader(TEXT("Editor"), EditorIconDefines);
    EditorIconRenderPass = std::make_shared<FEditorIconRenderPass>(IconShaderName);

    CreateVertexPixelShader(TEXT("HeightFog"), nullptr);
    FogRenderPass = std::make_shared<FFogRenderPass>(TEXT("HeightFog"));

    CreateVertexPixelShader(TEXT("Shadow"), nullptr);
    ShadowRenderPass = std::make_shared<FShadowRenderPass>(TEXT("Shadow"));

    CreateVertexPixelShader(TEXT("LightDepth"), nullptr);

    // init shadow resource
    FShadowResourceFactory::Initialize(graphics->Device);
}

void FRenderer::PrepareShader(const FName InShaderName)
{
    ShaderPrograms[InShaderName]->Bind();

    BindConstantBuffers(InShaderName);
}

void FRenderer::BindConstantBuffers(const FName InShaderName)
{
    TMap<FShaderConstantKey, uint32> curShaderBindedConstant = ShaderConstantNameAndSlots[InShaderName];
    for (const auto item : curShaderBindedConstant)
    {
        auto curConstantBuffer = RenderResourceManager->GetConstantBuffer(item.Key.ConstantName);
        if (item.Key.ShaderType == EShaderStage::VS)
        {
            if (curConstantBuffer)
                Graphics->DeviceContext->VSSetConstantBuffers(item.Value, 1, &curConstantBuffer);
        }
        else if (item.Key.ShaderType == EShaderStage::PS)
        {
            if (curConstantBuffer)
                Graphics->DeviceContext->PSSetConstantBuffers(item.Value, 1, &curConstantBuffer);
        }
    }
}

void FRenderer::CreateMappedCB(TMap<FShaderConstantKey, uint32>& ShaderStageToCB, const TArray<FConstantBufferInfo>& CBArray, const EShaderStage Stage) const
{
    for (const FConstantBufferInfo& item : CBArray)
    {
        ShaderStageToCB[{Stage, item.Name}] = item.BindSlot;
        if (RenderResourceManager->GetConstantBuffer(item.Name) == nullptr)
        {
            ID3D11Buffer* ConstantBuffer = RenderResourceManager->CreateConstantBuffer(item.ByteWidth);
            RenderResourceManager->AddOrSetConstantBuffer(item.Name, ConstantBuffer);
        }
    }
}

void FRenderer::Release()
{
    RenderResourceManager->ReleaseResources();
    
    delete RenderResourceManager;
    RenderResourceManager = nullptr;
    
    for (const auto item : ShaderPrograms)
    {
        item.Value->Release();
    }
}

void FRenderer::CreateVertexPixelShader(const FString& InPrefix, D3D_SHADER_MACRO* pDefines)
{
    FString Prefix = InPrefix;
    if (pDefines != nullptr)
    {
#if USE_WIDECHAR
        Prefix += ConvertAnsiToWchar(pDefines->Name);
#else
        Prefix += pDefines->Name;
#endif
    }
    // 접미사를 각각 붙여서 전체 파일명 생성
    const FString VertexShaderFile = InPrefix + TEXT("VertexShader.hlsl");
    const FString PixelShaderFile  = InPrefix + TEXT("PixelShader.hlsl");

    const FString VertexShaderName = Prefix+ TEXT("VertexShader.hlsl");
    const FString PixelShaderName = Prefix+ TEXT("PixelShader.hlsl");
    
    RenderResourceManager->CreateVertexShader(VertexShaderName, VertexShaderFile, pDefines);
    RenderResourceManager->CreatePixelShader(PixelShaderName, PixelShaderFile, pDefines);

    ID3DBlob* VertexShaderBlob = RenderResourceManager->GetVertexShaderBlob(VertexShaderName);
    
    TArray<FConstantBufferInfo> VertexStaticMeshConstant;
    ID3D11InputLayout* InputLayout = nullptr;
    Graphics->ExtractVertexShaderInfo(VertexShaderBlob, VertexStaticMeshConstant, InputLayout);
    RenderResourceManager->AddOrSetInputLayout(VertexShaderName, InputLayout);

    ID3DBlob* PixelShaderBlob = RenderResourceManager->GetPixelShaderBlob(PixelShaderName);
    TArray<FConstantBufferInfo> PixelStaticMeshConstant;
    Graphics->ExtractPixelShaderInfo(PixelShaderBlob, PixelStaticMeshConstant);
    
    TMap<FShaderConstantKey, uint32> ShaderStageToCB;

    CreateMappedCB(ShaderStageToCB, VertexStaticMeshConstant, EShaderStage::VS);  
    CreateMappedCB(ShaderStageToCB, PixelStaticMeshConstant, EShaderStage::PS);
    
    MappingVSPSInputLayout(Prefix, VertexShaderName, PixelShaderName, VertexShaderName);
    MappingVSPSCBSlot(Prefix, ShaderStageToCB);
}

#pragma region Shader

void FRenderer::CreateComputeShader()
{
    ID3DBlob* CSBlob_LightCulling = nullptr;
    
    ID3D11ComputeShader* ComputeShader = RenderResourceManager->GetComputeShader(TEXT("TileLightCulling"));
    
    if (ComputeShader == nullptr)
    {
        Graphics->CreateComputeShader(TEXT("TileLightCulling.compute"), nullptr, &CSBlob_LightCulling, &ComputeShader);
    }
    else
    {
        FGraphicsDevice::CompileComputeShader(TEXT("TileLightCulling.compute"), nullptr,  &CSBlob_LightCulling);
    }
    RenderResourceManager->AddOrSetComputeShader(TEXT("TileLightCulling"), ComputeShader);
    
    TArray<FConstantBufferInfo> LightCullingComputeConstant;
    Graphics->ExtractPixelShaderInfo(CSBlob_LightCulling, LightCullingComputeConstant);
    
    TMap<FShaderConstantKey, uint32> ShaderStageToCB;

    for (const FConstantBufferInfo item : LightCullingComputeConstant)
    {
        ShaderStageToCB[{EShaderStage::CS, item.Name}] = item.BindSlot;
        if (RenderResourceManager->GetConstantBuffer(item.Name) == nullptr)
        {
            ID3D11Buffer* ConstantBuffer = RenderResourceManager->CreateConstantBuffer(item.ByteWidth);
            RenderResourceManager->AddOrSetConstantBuffer(item.Name, ConstantBuffer);
        }
    }

    MappingVSPSCBSlot(TEXT("TileLightCulling"), ShaderStageToCB);
    
    ComputeTileLightCulling = std::make_shared<FComputeTileLightCulling>(TEXT("TileLightCulling"));

    SAFE_RELEASE(CSBlob_LightCulling)
}

void FRenderer::Render(UWorld* World, const std::shared_ptr<FEditorViewportClient>& ActiveViewport)
{
    SetViewMode(ActiveViewport->GetViewMode());
    Graphics->DeviceContext->RSSetViewports(1, &ActiveViewport->GetD3DViewport());

    if (ActiveViewport->GetViewMode() != EViewModeIndex::VMI_Wireframe
        && ActiveViewport->GetViewMode() != EViewModeIndex::VMI_Normal
        && ActiveViewport->GetViewMode() != EViewModeIndex::VMI_Depth
        && ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_Fog))
    {
        FogRenderPass->PrePrepare(); //fog 렌더 여부 결정 및 준비
    }
    //값을 써줄때 
    
    ComputeTileLightCulling->Dispatch(ActiveViewport);
    
    if (ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_Primitives))
    {
        ShadowRenderPass->Prepare(ActiveViewport);
        ShadowRenderPass->Execute(ActiveViewport);
        //TODO : FLAG로 나누기
        if (CurrentViewMode  == EViewModeIndex::VMI_Lit_Goroud)
        {
            GoroudRenderPass->Prepare(ActiveViewport);
            GoroudRenderPass->Execute(ActiveViewport);
        }
        else if (CurrentViewMode  == EViewModeIndex::VMI_Lit_Lambert)
        {
            LambertRenderPass->Prepare(ActiveViewport);
            LambertRenderPass->Execute(ActiveViewport);
        }
        else
        {
            PhongRenderPass->Prepare(ActiveViewport);
            PhongRenderPass->Execute(ActiveViewport);
        }
    }

    if (FogRenderPass->ShouldRender())
    {
        FogRenderPass->Prepare(ActiveViewport);
        FogRenderPass->Execute(ActiveViewport);
    }

    LineBatchRenderPass->Prepare(ActiveViewport);
    LineBatchRenderPass->Execute(ActiveViewport);

    if (ActiveViewport->GetViewMode() == EViewModeIndex::VMI_Depth)
    {
        DebugDepthRenderPass->Prepare(ActiveViewport);
        DebugDepthRenderPass->Execute(ActiveViewport);
    }
    
    EditorIconRenderPass->Prepare(ActiveViewport);
    EditorIconRenderPass->Execute(ActiveViewport);

    if (!World->GetSelectedActors().IsEmpty())
    {
        GizmoRenderPass->Prepare(ActiveViewport);
        GizmoRenderPass->Execute(ActiveViewport);
    }
}

void FRenderer::ClearRenderObjects() const
{
    GoroudRenderPass->ClearRenderObjects();
    LambertRenderPass->ClearRenderObjects();
    PhongRenderPass->ClearRenderObjects();
    LineBatchRenderPass->ClearRenderObjects();
    GizmoRenderPass->ClearRenderObjects();
    DebugDepthRenderPass->ClearRenderObjects();
    EditorIconRenderPass->ClearRenderObjects();
    ShadowRenderPass->ClearRenderObjects();
}

void FRenderer::SetViewMode(const EViewModeIndex evi)
{
    switch (evi)
    {
    case EViewModeIndex::VMI_Lit_Goroud:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Lit_Goroud;
        //TODO : Light 받는 거
        bIsLit = true;
        bIsNormal = false;
        break;
    case EViewModeIndex::VMI_Lit_Lambert:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Lit_Lambert;
        bIsLit = true;
        bIsNormal = false;
        break;
    case EViewModeIndex::VMI_Lit_Phong:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Lit_Phong;
        bIsLit = true;
        bIsNormal = false;
        break;
    case EViewModeIndex::VMI_Wireframe:
        CurrentRasterizerState = ERasterizerState::WireFrame;
        CurrentViewMode = VMI_Wireframe;
        bIsLit = false;
        bIsNormal = false;
        break;
    case EViewModeIndex::VMI_Unlit:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Unlit;
        //TODO : Light 안 받는 거
        bIsLit = false;
        bIsNormal = false;
        break;
    case EViewModeIndex::VMI_Depth:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Depth;
        break;
    case EViewModeIndex::VMI_Normal:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Normal;
        bIsLit = false;
        bIsNormal = true;
        break;
    default:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Lit_Phong;
        bIsLit = true;
        bIsNormal = false;
        break;
    }
}

void FRenderer::AddRenderObjectsToRenderPass(UWorld* InWorld) const
{
    ComputeTileLightCulling->AddRenderObjectsToRenderPass(InWorld);

    if (CurrentViewMode == VMI_Lit_Goroud)
    {
        GoroudRenderPass->AddRenderObjectsToRenderPass(InWorld);
    }
    else if (CurrentViewMode == VMI_Lit_Lambert)
    {
        LambertRenderPass->AddRenderObjectsToRenderPass(InWorld);
    }
    else
    {
        PhongRenderPass->AddRenderObjectsToRenderPass(InWorld);
    }
    
    GizmoRenderPass->AddRenderObjectsToRenderPass(InWorld);
    EditorIconRenderPass->AddRenderObjectsToRenderPass(InWorld);
    ShadowRenderPass->AddRenderObjectsToRenderPass(InWorld);
}

void FRenderer::MappingVSPSInputLayout(const FName InShaderProgramName, FName VSName, FName PSName, FName InInputLayoutName)
{
    ShaderPrograms.Add(InShaderProgramName, std::make_shared<FShaderProgram>(VSName, PSName, InInputLayoutName));
}

void FRenderer::MappingVSPSCBSlot(const FName InShaderName, const TMap<FShaderConstantKey, uint32>& MappedConstants)
{
    ShaderConstantNameAndSlots.Add(InShaderName, MappedConstants);
}

void FRenderer::MappingVBTopology(const FName InObjectName, const FName InVBName, const uint32 InStride, const uint32 InNumVertices, const D3D11_PRIMITIVE_TOPOLOGY InTopology)
{
    if (VBIBTopologyMappings.Contains(InObjectName) == false)
    {
        VBIBTopologyMappings[InObjectName] = std::make_shared<FVBIBTopologyMapping>();
    }
    VBIBTopologyMappings[InObjectName]->MappingVertexBuffer(InVBName, InStride, InNumVertices, InTopology);
}

void FRenderer::MappingIB(const FName InObjectName, const FName InIBName, const uint32 InNumIndices)
{
    if (VBIBTopologyMappings.Contains(InObjectName) == false)
    {
        VBIBTopologyMappings[InObjectName] = std::make_shared<FVBIBTopologyMapping>();
    }
    VBIBTopologyMappings[InObjectName]->MappingIndexBuffer(InIBName, InNumIndices);
}
