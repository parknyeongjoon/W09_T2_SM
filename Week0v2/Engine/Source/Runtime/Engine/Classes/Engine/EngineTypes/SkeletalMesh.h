#pragma once

#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "Container/Array.h"
#include "Container/String.h"

struct FBoneInfo
{
    FString Name;
    int32 ParentIndex;
    FMatrix LocalTransform;
    FMatrix GlobalReferencePose;
    FMatrix InverseBindPose;
};

struct FSkinnedVertex
{
    FVector Position;
    FVector Normal;
    FVector2D UV;
    int BoneIndices[4] = { -1, -1, -1, -1 };
    float BoneWeights[4] = { 0.f, 0.f, 0.f, 0.f };
};

struct FSkeletalMeshData
{
    TArray<FBoneInfo> Bones;
    TArray<FSkinnedVertex> Vertices;
    TArray<uint32> Indices;
    TArray<FMatrix> FinalBoneMatrices;
};
