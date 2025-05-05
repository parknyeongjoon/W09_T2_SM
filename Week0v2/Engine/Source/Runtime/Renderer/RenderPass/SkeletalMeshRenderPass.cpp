#include "SkeletalMeshRenderPass.h"
#include "LaunchEngineLoop.h"
#include "ShowFlags.h"
#include "Actors/ASkeletalMeshActor.h"
#include "Engine/FEditorStateManager.h"
#include "Engine/World.h"
#include "Renderer/Renderer.h"
#include "Renderer/VBIBTopologyMapping.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "UObject/UObjectIterator.h"

class UGizmoBaseComponent;
class USceneComponent;

FSkeletalMeshRenderPass::FSkeletalMeshRenderPass(const FName& InShaderName) : FBaseRenderPass(InShaderName)
{
    CreateConstant();
}

void FSkeletalMeshRenderPass::AddRenderObjectsToRenderPass()
{
    SkeletalMeshComponents.Empty();
    
    for (USceneComponent* SceneComponent : TObjectRange<USceneComponent>())
    {
        if (SceneComponent->GetWorld() != GEngine->GetWorld())
            continue;
                
        if (auto* SkelComp = Cast<USkeletalMeshComponent>(SceneComponent))
        {
            if (SkelComp->IsSetSkeletalMesh())
                SkeletalMeshComponents.Add(SkelComp);
        }
    }
}

void FSkeletalMeshRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FBaseRenderPass::Prepare(InViewportClient);
    
    const FRenderer& Renderer = FEngineLoop::Renderer;
    FGraphicsDevice& Graphics = FEngineLoop::GraphicDevice;

    Graphics.DeviceContext->OMSetDepthStencilState(Renderer.GetDepthStencilState(EDepthStencilState::DepthNone), 0);
    Graphics.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics.DeviceContext->RSSetState(Renderer.GetCurrentRasterizerState());

    const auto CurrRtv = Graphics.GetCurrentRenderTargetView();
    if (CurrRtv != nullptr)
        Graphics.DeviceContext->OMSetRenderTargets(1, &CurrRtv, Graphics.DepthStencilView);

    Graphics.DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void FSkeletalMeshRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FRenderer& Renderer = FEngineLoop::Renderer;
    FGraphicsDevice& Graphics = FEngineLoop::GraphicDevice;
    
    FMatrix View = FMatrix::Identity;
    FMatrix Proj = FMatrix::Identity;
    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);
    if (curEditorViewportClient != nullptr)
    {
        View = curEditorViewportClient->GetViewMatrix();
        Proj = curEditorViewportClient->GetProjectionMatrix();
    }

    for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
    {
        const FMatrix Model = SkeletalMeshComponent->GetWorldMatrix();
        UpdateMatrixConstants(SkeletalMeshComponent, View, Proj);
        
        std::shared_ptr<FEditorViewportClient> currEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);

        // AABB
        if (currEditorViewportClient->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::Type::SF_AABB))
        {
            if ( !GEngine->GetWorld()->GetSelectedActors().IsEmpty() && *GEngine->GetWorld()->GetSelectedActors().begin() == SkeletalMeshComponent->GetOwner())
            {
                UPrimitiveBatch::GetInstance().AddAABB(
                    SkeletalMeshComponent->GetBoundingBox(),
                    SkeletalMeshComponent->GetWorldLocation(),
                    Model
                );
            }
        }

        FSkeletalMeshData* meshData = SkeletalMeshComponent->GetSkeletalMesh()->GetMeshData();
        
        // VIBuffer Bind
        const std::shared_ptr<FVBIBTopologyMapping> VBIBTopMappingInfo = Renderer.GetVBIBTopologyMapping(meshData->Name);
        VBIBTopMappingInfo->Bind();

        // Draw
        Graphics.DeviceContext->DrawIndexed(VBIBTopMappingInfo->GetNumIndices(), 0,0);

        // if (meshData->MaterialSubsets.Num() == 0)
        // {
        //     Graphics.DeviceContext->DrawIndexed(VBIBTopMappingInfo->GetNumIndices(), 0,0);
        // }

        // SubSet마다 Material Update 및 Draw
        // for (int subMeshIndex = 0; subMeshIndex < renderData->MaterialSubsets.Num(); ++subMeshIndex)
        // {
        //     const int materialIndex = renderData->MaterialSubsets[subMeshIndex].MaterialIndex;
        //     
        //     UpdateMaterialConstants(staticMeshComp->GetMaterial(materialIndex)->GetMaterialInfo());
        //
        //     // index draw
        //     const uint64 startIndex = renderData->MaterialSubsets[subMeshIndex].IndexStart;
        //     const uint64 indexCount = renderData->MaterialSubsets[subMeshIndex].IndexCount;
        //     Graphics.DeviceContext->DrawIndexed(indexCount, startIndex, 0);
        // }
    }
}

void FSkeletalMeshRenderPass::ClearRenderObjects()
{
    SkeletalMeshComponents.Empty();
}

void FSkeletalMeshRenderPass::CreateConstant()
{
    // D3D11_BUFFER_DESC constdesc = {};
    // constdesc.ByteWidth = sizeof(FLightingConstants);
    // constdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    // constdesc.Usage = D3D11_USAGE_DYNAMIC;
    // constdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    // Graphics.Device->CreateBuffer(&constdesc, nullptr, &LightConstantBuffer);
}

void FSkeletalMeshRenderPass::UpdateMatrixConstants(USkeletalMeshComponent* InSkeletalMeshComponent, const FMatrix& InView, const FMatrix& InProjection)
{
    FRenderResourceManager* renderResourceManager = FEngineLoop::Renderer.GetResourceManager();
    // MVP Update
    const FMatrix Model = InSkeletalMeshComponent->GetWorldMatrix();
    const FMatrix NormalMatrix = FMatrix::Transpose(FMatrix::Inverse(Model));
    
    FSkeletalMatrixConstant MatrixConstants;
    MatrixConstants.Model = Model;
    MatrixConstants.ViewProj = InView * InProjection;
    MatrixConstants.MInverseTranspose = NormalMatrix;
    renderResourceManager->UpdateConstantBuffer(TEXT("FSkeletalMatrixConstant"), &MatrixConstants);
}

void FSkeletalMeshRenderPass::UpdateConstant()
{
    const FGraphicsDevice& Graphics = FEngineLoop::GraphicDevice;
    FRenderResourceManager* RenderResourceManager = FEngineLoop::Renderer.GetResourceManager();
    
    for (USkeletalMeshComponent* SkelComp : SkeletalMeshComponents)
    {
        // 1) CPU 스키닝
        // SkelComp->UpdateBoneTransforms();
        // SkelComp->SkinMeshCPU();
        // SkelComp->SubmitBuffersToGPU();

        // 2) 본 상수 버퍼 업데이트
        // FBoneMatricesConstant boneCB;
        // int nb = SkelComp->GetNumBones();
        // for (int i = 0; i < nb; ++i)
        //     boneCB.BoneMatrices[i] = SkelComp->FinalBoneMatrices[i];
        // boneCB.NumBones = nb;
        //
        // D3D11_MAPPED_SUBRESOURCE Mapped;
        // Graphics.DeviceContext->Map(BoneMatrixCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
        // memcpy(Mapped.pData, &boneCB, sizeof(boneCB));
        // Graphics.DeviceContext->Unmap(BoneMatrixCB, 0);
        // Graphics.DeviceContext->VSSetConstantBuffers(1, 1, &BoneMatrixCB);

        // 3) VB/IB 바인딩
        // auto mapping = GEngineLoop.Renderer
        //     .GetVBIBTopologyMapping(SkelComp->GetVBIBTopologyMappingName());
        // mapping->Bind();
        //
        // // 4) DrawIndexed 호출
        // G.DeviceContext->DrawIndexed(
        //     mapping->GetNumIndices(),  // 인덱스 개수
        //     0,                         // StartIndex
        //     0                          // BaseVertex
        // );
    }
    
    //renderResourceManager->UpdateConstantBuffer(LetterBoxConstantBuffer, &LetterBoxConstants);
    //Graphics.DeviceContext->PSSetConstantBuffers(0, 1, &LetterBoxConstantBuffer);
}
