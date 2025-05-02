#include "PrimitiveBatch.h"
#include "EditorEngine.h"
#include "UnrealEd/EditorViewportClient.h"
extern UEditorEngine* GEngine;

UPrimitiveBatch::UPrimitiveBatch()
{
    GenerateGrid(5, 5000);
}

UPrimitiveBatch::~UPrimitiveBatch()
{
}

void UPrimitiveBatch::Release()
{
}

void UPrimitiveBatch::GenerateGrid(float spacing, int gridCount)
{
    GridParam.GridSpacing = spacing;
    GridParam.GridCount = gridCount;
    GridParam.GridOrigin = { 0,0,0 };
}

// void UPrimitiveBatch::RenderBatch(ID3D11Buffer* ConstantBuffer, const FMatrix& View, const FMatrix& Projection)
// {
//     UEditorEngine::renderer.PrepareLineShader();
//
//     InitializeVertexBuffer();
//
//     FMatrix Model = FMatrix::Identity;
//     FMatrix ViewProj = View * Projection;
//     FMatrix NormalMatrix = FMatrix::Transpose(FMatrix::Inverse(Model));
//     UEditorEngine::renderer.GetConstantBufferUpdater().UpdateConstant(ConstantBuffer, Model, ViewProj, NormalMatrix, FVector4(0, 0, 0, 0), false);
//     UEditorEngine::renderer.UpdateGridConstantBuffer(GridParam);
//
//     UpdateBoundingBoxResources();
//     UpdateConeResources();
//     UpdateOBBResources();
//     int boundingBoxSize = static_cast<int>(BoundingBoxes.Num());
//     int coneSize = static_cast<int>(Cones.Num());
//     int obbSize = static_cast<int>(OrientedBoundingBoxes.Num());
//     UEditorEngine::renderer.UpdateLinePrimitveCountBuffer(boundingBoxSize, coneSize);
//     UEditorEngine::renderer.RenderBatch(GridParam, pVertexBuffer, boundingBoxSize, coneSize, ConeSegmentCount, obbSize);
//     BoundingBoxes.Empty();
//     Cones.Empty();
//     OrientedBoundingBoxes.Empty();
//     UEditorEngine::renderer.PrepareShader();
// }

void UPrimitiveBatch::AddAABB(const FBoundingBox& localAABB, const FVector& center, const FMatrix& modelMatrix)
{
    FVector localVertices[8] = {
         { localAABB.min.X, localAABB.min.Y, localAABB.min.Z },
         { localAABB.max.X, localAABB.min.Y, localAABB.min.Z },
         { localAABB.min.X, localAABB.max.Y, localAABB.min.Z },
         { localAABB.max.X, localAABB.max.Y, localAABB.min.Z },
         { localAABB.min.X, localAABB.min.Y, localAABB.max.Z },
         { localAABB.max.X, localAABB.min.Y, localAABB.max.Z },
         { localAABB.min.X, localAABB.max.Y, localAABB.max.Z },
         { localAABB.max.X, localAABB.max.Y, localAABB.max.Z }
    };

    FVector worldVertices[8];
    worldVertices[0] = modelMatrix.TransformPosition(localVertices[0]);

    FVector min = worldVertices[0], max = worldVertices[0];

    // 첫 번째 값을 제외한 나머지 버텍스를 변환하고 min/max 계산
    for (int i = 1; i < 8; ++i)
    {
        worldVertices[i] = modelMatrix.TransformPosition(localVertices[i]);

        min.X = (worldVertices[i].X < min.X) ? worldVertices[i].X : min.X;
        min.Y = (worldVertices[i].Y < min.Y) ? worldVertices[i].Y : min.Y;
        min.Z = (worldVertices[i].Z < min.Z) ? worldVertices[i].Z : min.Z;

        max.X = (worldVertices[i].X > max.X) ? worldVertices[i].X : max.X;
        max.Y = (worldVertices[i].Y > max.Y) ? worldVertices[i].Y : max.Y;
        max.Z = (worldVertices[i].Z > max.Z) ? worldVertices[i].Z : max.Z;
    }
    FBoundingBox BoundingBox;
    BoundingBox.min = min;
    BoundingBox.max = max;
    BoundingBoxes.Add(BoundingBox);
}
void UPrimitiveBatch::AddOBB(const FBoundingBox& localAABB, const FVector& center, const FMatrix& modelMatrix)
{
    // 1) 로컬 AABB의 8개 꼭짓점
    FVector localVertices[8] =
    {
        { localAABB.min.X, localAABB.min.Y, localAABB.min.Z },
        { localAABB.max.X, localAABB.min.Y, localAABB.min.Z },
        { localAABB.min.X, localAABB.max.Y, localAABB.min.Z },
        { localAABB.max.X, localAABB.max.Y, localAABB.min.Z },
        { localAABB.min.X, localAABB.min.Y, localAABB.max.Z },
        { localAABB.max.X, localAABB.min.Y, localAABB.max.Z },
        { localAABB.min.X, localAABB.max.Y, localAABB.max.Z },
        { localAABB.max.X, localAABB.max.Y, localAABB.max.Z }
    };
    
    FOBB faceBB;
    for (int32 i = 0; i < 8; ++i) {
        // 모델 매트릭스로 점을 변환 후, center를 더해준다.
        faceBB.corners[i] =  modelMatrix.TransformPosition(localVertices[i]);
    }

    OrientedBoundingBoxes.Add(faceBB);

}

void UPrimitiveBatch::AddCone(const FVector& start, float radius, const FVector& end, int segments, const FVector4& color)
{
    ConeSegmentCount = segments;
    FCone cone;
    cone.ConeApex = start;
    cone.ConeBaseCenter = end;
    cone.ConeRadius = radius;
    cone.ConeHeight = (start-end).Magnitude();
    cone.Color = color;
    cone.ConeSegmentCount = ConeSegmentCount;
    Cones.Add(cone);
}

void UPrimitiveBatch::AddSphere(const FVector& center, float radius, const FVector4& color)
{
    FSphere Sphere;
    Sphere.Center = center;
    Sphere.Radius = radius;
    Sphere.Color = color;
    Spheres.Add(Sphere);
}

void UPrimitiveBatch::AddLine(const FVector& StartPos, const FVector& Direction, const float Length, const FVector4& Color)
{
    FLine Line;
    Line.Start = StartPos;
    Line.Direction = Direction;
    Line.Length = Length;
    Line.Color = Color;
    Lines.Add(Line);
}

void UPrimitiveBatch::AddCapsule(const FVector& Center, const FVector& UpVector, float HalfHeight, float Radius, const FVector4& Color)
{
    FCapsule capsule;
    capsule.Center = Center;
    capsule.UpVector = UpVector;
    capsule.HalfHeight = HalfHeight;
    capsule.Radius = Radius;
    capsule.Color = Color;
    Capsules.Add(capsule);
}


