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
};

struct FSkinWeight
{
    int   BoneIndices[4];
    float BoneWeights[4];
};

struct FSkeletalMeshData
{
    FString Name;
    TArray<FBoneInfo> Bones;
    TArray<FSkinnedVertex> Vertices;
    TArray<uint32> Indices;
    TArray<FSkinWeight> Weights;
};