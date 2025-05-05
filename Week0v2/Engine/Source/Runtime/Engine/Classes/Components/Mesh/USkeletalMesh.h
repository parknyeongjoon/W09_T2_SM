#pragma once
#include "Define.h"
#include "Engine/EngineTypes/SkeletalMesh.h"
#include "UObject/Object.h"

class USkeletalMesh : public UObject
{
public:
    FSkeletalMeshData SkeletalMeshData;
    FBoundingBox      BoundingBox;

public:
    bool SetData(const FSkeletalMeshData& InData);
    FSkeletalMeshData* GetMeshData() { return &SkeletalMeshData; }
    
    void CreateRenderBuffers() const;
    void CalculateBoundingBox();
};
