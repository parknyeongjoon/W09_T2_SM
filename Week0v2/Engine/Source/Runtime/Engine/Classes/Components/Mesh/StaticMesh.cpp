#include "StaticMesh.h"

#include "LaunchEngineLoop.h"
#include "Engine/Loader/OBJ/FLoaderOBJ.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderResourceManager.h"
#include "UObject/ObjectFactory.h"

UStaticMesh::UStaticMesh()
{

}

UStaticMesh::~UStaticMesh()
{
}

uint32 UStaticMesh::GetMaterialIndex(FName MaterialSlotName) const
{
    for (uint32 materialIndex = 0; materialIndex < materials.Num(); materialIndex++) {
        if (materials[materialIndex]->MaterialSlotName == MaterialSlotName)
            return materialIndex;
    }

    return -1;
}

void UStaticMesh::GetUsedMaterials(TArray<UMaterial*>& Out) const
{
    for (const FStaticMaterial* Material : materials)
    {
        Out.Emplace(Material->Material);
    }
}

void UStaticMesh::SetData(OBJ::FStaticMeshRenderData* renderData)
{
    staticMeshRenderData = renderData;

    ID3D11Buffer* VB = nullptr; 
    ID3D11Buffer* IB = nullptr;

    const uint32 verticeNum = staticMeshRenderData->Vertices.Num();
    if (verticeNum <= 0) return;

    FRenderResourceManager* renderResourceManager = GEngineLoop.Renderer.GetResourceManager();
    VB = renderResourceManager->CreateImmutableVertexBuffer<FVertexSimple>(staticMeshRenderData->Vertices);
    renderResourceManager->AddOrSetVertexBuffer(staticMeshRenderData->DisplayName, VB);
    GEngineLoop.Renderer.MappingVBTopology(staticMeshRenderData->DisplayName, staticMeshRenderData->DisplayName, sizeof(FVertexSimple), verticeNum);
    
    const uint32 indexNum = staticMeshRenderData->Indices.Num();
    if (indexNum > 0)
    {
        IB = renderResourceManager->CreateIndexBuffer(staticMeshRenderData->Indices);
        renderResourceManager->AddOrSetIndexBuffer(staticMeshRenderData->DisplayName, IB);
    }
    GEngineLoop.Renderer.MappingVBTopology(staticMeshRenderData->DisplayName, staticMeshRenderData->DisplayName, sizeof(FVertexSimple), verticeNum);
    GEngineLoop.Renderer.MappingIB(staticMeshRenderData->DisplayName, staticMeshRenderData->DisplayName, indexNum);
    
    for (int materialIndex = 0; materialIndex < staticMeshRenderData->Materials.Num(); materialIndex++) {
        FStaticMaterial* newMaterialSlot = new FStaticMaterial();
        UMaterial* newMaterial = FManagerOBJ::CreateMaterial(staticMeshRenderData->Materials[materialIndex]);

        newMaterialSlot->Material = newMaterial;
        newMaterialSlot->MaterialSlotName = staticMeshRenderData->Materials[materialIndex].MTLName;

        materials.Add(newMaterialSlot);
    }
}
