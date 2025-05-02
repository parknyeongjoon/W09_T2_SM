#include "LineBatchRenderPass.h"

#include "Define.h"
#include "EditorEngine.h"
#include "Renderer/VBIBTopologyMapping.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "Engine/World.h"
#include <Math/JungleMath.h>
#include "LevelEditor/SLevelEditor.h"
#include "Components/LightComponents/PointLightComponent.h"
#include "Components/LightComponents/SpotLightComponent.h"
#include "Components/PrimitiveComponents/Physics/UShapeComponent.h"
#include "Components/PrimitiveComponents/Physics/UCapsuleShapeComponent.h"
#include "Components/PrimitiveComponents/Physics/USphereShapeComponent.h"
#include "Components/PrimitiveComponents/Physics/UBoxShapeComponent.h"
#include "PropertyEditor/ShowFlags.h"
class USpotLightComponent;
extern UEditorEngine* GEngine;

FLineBatchRenderPass::FLineBatchRenderPass(const FName& InShaderName)
    : FBaseRenderPass(InShaderName)
{
    FSimpleVertex vertices[2]{ {0}, {0} };

    FRenderResourceManager* renderResourceManager = GEngine->renderer.GetResourceManager();
    ID3D11Buffer* pVertexBuffer = renderResourceManager->CreateStaticVertexBuffer<FSimpleVertex>(vertices, 2);
    renderResourceManager->AddOrSetVertexBuffer(TEXT("LineBatchVB"), pVertexBuffer);

    GEngine->renderer.MappingVBTopology(TEXT("LineBatch"), TEXT("LineBatchVB"), sizeof(FSimpleVertex), 2, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    VBIBTopologyMappingName = TEXT("LineBatch");
}

void FLineBatchRenderPass::AddRenderObjectsToRenderPass(UWorld* InWorld)
{
    std::shared_ptr<FViewportClient> ViewportClient = 
        GEngine->GetLevelEditor()->GetActiveViewportClient();
    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = 
        std::dynamic_pointer_cast<FEditorViewportClient>(ViewportClient);
    if (!(curEditorViewportClient->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::Type::SF_AABB)))
        return;

    UPrimitiveBatch& PrimitiveBatch = UPrimitiveBatch::GetInstance();
    for (const AActor* actor : InWorld->GetActors())
    {
        for (const UActorComponent* actorComp : actor->GetComponents())
        {
            if (UShapeComponent* pShapeComponent = Cast<UShapeComponent>(actorComp))
            {
                const FBoundingBox& Box = pShapeComponent->GetBroadAABB();
                FVector Center = pShapeComponent->GetWorldLocation();

                PrimitiveBatch.AddAABB(Box, Center, FMatrix::Identity);

                const FShapeInfo* BaseShapeInfo = pShapeComponent->GetShapeInfo();

                if (!BaseShapeInfo)
                    continue;

                switch (BaseShapeInfo->Type)
                {
                case EShapeType::Box:
                {
                    const FBoxShapeInfo* BoxInfo = static_cast<const FBoxShapeInfo*>(BaseShapeInfo);

                    FVector BoxExtent = BoxInfo->Extent;
                    FVector Center = BoxInfo->Center;
                    FMatrix WorldMatrix = BoxInfo->WorldMatrix;

                    FBoundingBox localOBB;
                    localOBB.min = FVector(-BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z);
                    localOBB.max = FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z);

                    PrimitiveBatch.AddOBB(localOBB, Center, WorldMatrix);
                    break;
                }
                case EShapeType::Sphere:
                {
                    const FSphereShapeInfo* SphereInfo = static_cast<const FSphereShapeInfo*>(BaseShapeInfo);

                    FVector Center = SphereInfo->Center;
                    float Radius = SphereInfo->Radius;
                    FVector4 Color = FVector4(0.0f, 1.0f, 0.0f, 1.0f); // 사용 안됨.

                    PrimitiveBatch.AddSphere(Center, Radius, Color);
                    break;
                }
                case EShapeType::Capsule:
                {
                    const FCapsuleShapeInfo* CapsuleInfo = static_cast<const FCapsuleShapeInfo*>(BaseShapeInfo);

                    FVector Center = CapsuleInfo->Center;
                    FVector UpV = CapsuleInfo->Up;
                    float Radius = CapsuleInfo->Radius;
                    float HalfHeight = CapsuleInfo->HalfHeight;
                    FVector4 Color = FVector4(0.0f, 1.0f, 0.0f, 1.0f); // 사용 안됨.

                    UpV.Normalize();

                    PrimitiveBatch.AddCapsule(Center, UpV, HalfHeight, Radius, Color);
                    break;
                }
                default:
                    break;
                }
            }
        }
    }
}

void FLineBatchRenderPass::Execute(const std::shared_ptr<FViewportClient> InViewportClient)
{
    FRenderer& Renderer = GEngine->renderer;
    FGraphicsDevice& Graphics = GEngine->graphicDevice;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    UPrimitiveBatch& PrimitveBatch = UPrimitiveBatch::GetInstance();

    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);

    // 쉐이더 내에서 한 번만 Update되어야하는 CB
    const FMatrix Model = FMatrix::Identity;

    FMatrixBuffer MVPConstant;
    MVPConstant.Model = Model;
    if (curEditorViewportClient != nullptr)
    {
        MVPConstant.ViewProj = curEditorViewportClient->GetViewMatrix() * curEditorViewportClient->GetProjectionMatrix();
    }

    renderResourceManager->UpdateConstantBuffer(TEXT("FMatrixBuffer"), &MVPConstant);

    const FGridParametersData GridParameters = PrimitveBatch.GetGridParameters();
    renderResourceManager->UpdateConstantBuffer(TEXT("FGridParametersData"), &GridParameters);

    UpdateBatchResources();

    FPrimitiveCounts PrimitiveCounts;
    PrimitiveCounts.ConeCount = PrimitveBatch.GetCones().Num();
    PrimitiveCounts.BoundingBoxCount = PrimitveBatch.GetBoundingBoxes().Num();
    PrimitiveCounts.SphereCount = PrimitveBatch.GetSpheres().Num();
    PrimitiveCounts.LineCount = PrimitveBatch.GetLines().Num();
    PrimitiveCounts.CapsuleCount = PrimitveBatch.GetCapsules().Num();
    PrimitiveCounts.OBBCount = PrimitveBatch.GetOrientedBoundingBoxes().Num();
    renderResourceManager->UpdateConstantBuffer(TEXT("FPrimitiveCounts"), &PrimitiveCounts);

    const std::shared_ptr<FVBIBTopologyMapping> VBIBTopologyMappingInfo = Renderer.GetVBIBTopologyMapping(VBIBTopologyMappingName);
    VBIBTopologyMappingInfo->Bind();

    const uint32 vertexCountPerInstance = 2;

    const uint32 NumSegmentsCircle = 16;
    const uint32 NumVerticalLines = 4;
    const uint32 NumHemisphereSegments = 8;

    const uint32 capsuleInstancePerCapsule =
        (NumSegmentsCircle * 2) + // Top/Bottom Circle
        NumVerticalLines +        // 세로선 4개
        (NumHemisphereSegments * 4) + // Top 반구 (0°/180°, 90°/270°)
        (NumHemisphereSegments * 4);  // Bottom 반구 (0°/180°, 90°/270°)

    const uint32 instanceCount =
        GridParameters.GridCount +
        3 +
        (PrimitveBatch.GetBoundingBoxes().Num() * 12) +
        (PrimitveBatch.GetCones().Num() * (2 * PrimitveBatch.GetConeSegmentCount())) +
        (PrimitveBatch.GetSpheres().Num() * 3 * 32) +
        (PrimitveBatch.GetLines().Num() * 2) +
        (12 * PrimitveBatch.GetOrientedBoundingBoxes().Num()) +
        (PrimitveBatch.GetCapsules().Num() * capsuleInstancePerCapsule);

    Graphics.DeviceContext->DrawInstanced(vertexCountPerInstance, instanceCount, 0, 0);


    PrimitveBatch.ClearBatchPrimitives();
}

void FLineBatchRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FBaseRenderPass::Prepare(InViewportClient);

    FRenderer& Renderer = GEngine->renderer;
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();

    Graphics.DeviceContext->RSSetState(Renderer.GetCurrentRasterizerState()); //레스터 라이저 상태 설정
    Graphics.DeviceContext->OMSetDepthStencilState(Renderer.GetDepthStencilState(EDepthStencilState::LessEqual), 0);
    
    const auto CurRTV = Graphics.GetCurrentRenderTargetView();
    Graphics.DeviceContext->OMSetRenderTargets(1, &CurRTV, Graphics.DepthStencilView); // 렌더 타겟 설정

    for (AActor* actor : GEngine->GetWorld()->GetSelectedActors())
    {
        ALight* Light = Cast<ALight>(actor);
        if (Light)
        {
            TArray<UActorComponent*> Comps = Light->GetComponents();
            for (const auto& Comp : Comps)
            {
                if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(Comp))
                {
                    const FMatrix Model = SpotLight->GetWorldMatrix();
                    if (SpotLight->GetOuterConeAngle() > 0)
                    {
                        UPrimitiveBatch::GetInstance().AddCone(
                            SpotLight->GetWorldLocation(),
                            tan(SpotLight->GetOuterConeAngle()) * 15.0f,
                            SpotLight->GetWorldLocation() + SpotLight->GetForwardVector() * 15.0f,
                            15,
                            SpotLight->GetLightColor()
                        );
                    }
                    if (SpotLight->GetInnerConeAngle() > 0)
                    {
                        UPrimitiveBatch::GetInstance().AddCone(
                            SpotLight->GetWorldLocation(),
                            tan(SpotLight->GetInnerConeAngle()) * 15.0f,
                            SpotLight->GetWorldLocation() + SpotLight->GetForwardVector() * 15.0f,
                            15,
                            SpotLight->GetLightColor()
                        );
                    }
                }
                else if (UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(Comp))
                {
                    FVector Right = DirectionalLight->GetRightVector();
                    for (int i = 0; i < 4; ++i)
                    {
                        UPrimitiveBatch::GetInstance().AddLine(
                            DirectionalLight->GetWorldLocation() + Right * (-1.5f + i),
                            DirectionalLight->GetForwardVector(),
                            15.0f,
                            DirectionalLight->GetLightColor()
                        );
                    }
                }
                if (UPointLightComponent* PointLight = Cast< UPointLightComponent>(Comp))
                {
                    if (PointLight->GetRadius() > 0)
                    {
                        UPrimitiveBatch::GetInstance().AddSphere(
                            PointLight->GetWorldLocation(),
                            PointLight->GetRadius(),
                            PointLight->GetLightColor()
                        );
                    }
                }
            }
        }
    }
    ID3D11ShaderResourceView* FBoundingBoxSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("BoundingBox"));
    Graphics.DeviceContext->VSSetShaderResources(3, 1, &FBoundingBoxSRV);
    ID3D11ShaderResourceView* FConeSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Cone"));
    Graphics.DeviceContext->VSSetShaderResources(4, 1, &FConeSRV);
    ID3D11ShaderResourceView* FOBBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("OBB"));
    Graphics.DeviceContext->VSSetShaderResources(5, 1, &FOBBSRV);
    ID3D11ShaderResourceView* FSphereSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Sphere"));
    Graphics.DeviceContext->VSSetShaderResources(6, 1, &FSphereSRV);
    ID3D11ShaderResourceView* FLineSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Line"));
    Graphics.DeviceContext->VSSetShaderResources(7, 1, &FLineSRV);
    ID3D11ShaderResourceView* FCapsuleSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Capsule"));
    Graphics.DeviceContext->VSSetShaderResources(8, 1, &FCapsuleSRV);
}

void FLineBatchRenderPass::ClearRenderObjects()
{
    ShapeComponents.Empty();
    CapsuleShapeComponents.Empty();
}

void FLineBatchRenderPass::UpdateBatchResources()
{
    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;

    UPrimitiveBatch& PrimitveBatch = UPrimitiveBatch::GetInstance();

    {
        if (PrimitveBatch.GetBoundingBoxes().Num() > PrimitveBatch.GetAllocatedBoundingBoxCapacity())
        {
            PrimitveBatch.SetAllocatedBoundingBoxCapacity(PrimitveBatch.GetBoundingBoxes().Num());

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FBoundingBox>(static_cast<uint32>(PrimitveBatch.GetAllocatedBoundingBoxCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(PrimitveBatch.GetAllocatedBoundingBoxCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("BoundingBox"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("BoundingBox"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("BoundingBox"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("BoundingBox"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, PrimitveBatch.GetBoundingBoxes());
        }
    }

    {
        if (UPrimitiveBatch::GetInstance().GetCones().Num() > UPrimitiveBatch::GetInstance().GetAllocatedConeCapacity())
        {
            UPrimitiveBatch::GetInstance().SetAllocatedConeCapacity(UPrimitiveBatch::GetInstance().GetCones().Num());

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FCone>(static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedConeCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedConeCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("Cone"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("Cone"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("Cone"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Cone"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, UPrimitiveBatch::GetInstance().GetCones());
        }
    }

    {
        if (UPrimitiveBatch::GetInstance().GetSpheres().Num() > UPrimitiveBatch::GetInstance().GetAllocatedSphereCapacity())
        {
            UPrimitiveBatch::GetInstance().SetAllocatedSphereCapacity(UPrimitiveBatch::GetInstance().GetSpheres().Num());

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FSphere>(static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedSphereCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedSphereCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("Sphere"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("Sphere"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("Sphere"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Sphere"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, UPrimitiveBatch::GetInstance().GetSpheres());
        }
    }

    {
        if (PrimitveBatch.GetLines().Num() > PrimitveBatch.GetAllocatedLineCapacity())
        {
            PrimitveBatch.SetAllocatedLineCapacity(PrimitveBatch.GetLines().Num());

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FLine>(static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedLineCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedLineCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("Line"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("Line"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("Line"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Line"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, UPrimitiveBatch::GetInstance().GetLines());
        }
    }
    {
        int32 CapsulesNum = PrimitveBatch.GetCapsules().Num();
        if (CapsulesNum > PrimitveBatch.GetAllocatedCapsuleCapacity())
        {
            PrimitveBatch.SetAllocatedCapsuleCapacity(CapsulesNum);

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FCapsule>(static_cast<uint32>(PrimitveBatch.GetAllocatedCapsuleCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(PrimitveBatch.GetAllocatedCapsuleCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("Capsule"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("Capsule"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("Capsule"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Capsule"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, PrimitveBatch.GetCapsules());
        }
    }

    {
        if (PrimitveBatch.GetOrientedBoundingBoxes().Num() > PrimitveBatch.GetAllocatedOBBCapacity())
        {
            PrimitveBatch.SetAllocatedOBBCapacity(PrimitveBatch.GetOrientedBoundingBoxes().Num());

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FOBB>(static_cast<uint32>(PrimitveBatch.GetAllocatedOBBCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(PrimitveBatch.GetAllocatedOBBCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("OBB"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("OBB"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("OBB"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("OBB"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, PrimitveBatch.GetOrientedBoundingBoxes());
        }
    }
}
