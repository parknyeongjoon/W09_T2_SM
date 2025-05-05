#include "USkeletalMesh.h"
#include "LaunchEngineLoop.h"
#include "Renderer/Renderer.h"

bool USkeletalMesh::SetData(const FSkeletalMeshData& InData)
{
    SkeletalMeshData = InData;
    CreateRenderBuffers();
    CalculateBoundingBox();
    return true;
}

void USkeletalMesh::CreateRenderBuffers() const
{
    ID3D11Buffer* VertexBuffer = nullptr;
    const uint32 verticeNum = SkeletalMeshData.Vertices.Num();
    if (verticeNum <= 0) return;

    FRenderResourceManager* renderResourceManager = FEngineLoop::Renderer.GetResourceManager();
    VertexBuffer = renderResourceManager->CreateDynamicVertexBuffer<FSkinnedVertex>(SkeletalMeshData.Vertices);
    renderResourceManager->AddOrSetVertexBuffer(SkeletalMeshData.Name, VertexBuffer);
    FEngineLoop::Renderer.MappingVBTopology(SkeletalMeshData.Name, SkeletalMeshData.Name, sizeof(FVertexSimple), verticeNum);
    
    const uint32 indexNum = SkeletalMeshData.Indices.Num();
    if (indexNum > 0)
    {
        ID3D11Buffer* IndexBuffer = nullptr;
        IndexBuffer = renderResourceManager->CreateIndexBuffer(SkeletalMeshData.Indices);
        renderResourceManager->AddOrSetIndexBuffer(SkeletalMeshData.Name, IndexBuffer);
    }
    FEngineLoop::Renderer.MappingIB(SkeletalMeshData.Name, SkeletalMeshData.Name, indexNum);
}

void USkeletalMesh::CalculateBoundingBox()
{
    if (SkeletalMeshData.Vertices.Num() == 0)
    {
        BoundingBox = FBoundingBox(FVector(0.0f), FVector(0.0f));
        return;
    }

    const auto& V0 = SkeletalMeshData.Vertices[0].Position;
    FVector minV = V0;
    FVector maxV = V0;

    for (const auto& SV : SkeletalMeshData.Vertices)
    {
        const FVector& P = SV.Position;
        minV.X = std::min(minV.X, P.X);
        minV.Y = std::min(minV.Y, P.Y);
        minV.Z = std::min(minV.Z, P.Z);

        maxV.X = std::max(maxV.X, P.X);
        maxV.Y = std::max(maxV.Y, P.Y);
        maxV.Z = std::max(maxV.Z, P.Z);
    }

    BoundingBox = FBoundingBox(minV, maxV);
}
