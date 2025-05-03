#pragma once

#include "Define.h"
#include "fbxsdk.h"
#include "Container/Array.h"
#include "Container/Map.h"
#include "Engine/EngineTypes/SkeletalMesh.h"

struct FLoaderFBX
{
    static bool LoadSkeletalMesh(const FString& FilePath, FSkeletalMeshData& OutData);
    static bool LoadStaticMesh(const FString& FilePath, OBJ::FStaticMeshRenderData& OutData);

private:
    static void ParseSkeletonRecursive(FbxNode* Node, TArray<FBoneInfo>& BoneList, int32 ParentIndex);
    static void ComputeGlobalBindPoses(TArray<FBoneInfo>& Bones);
    static void CollectMeshes(FbxNode* Node, TArray<FbxMesh*>& OutMeshes);
    static void ExtractMeshData(FbxMesh* Mesh, TArray<FSkinnedVertex>& OutVertices, TArray<uint32>& OutIndices, TMap<int32, TArray<int32>>& OutCP2Verts);
    static void ParseSkinWeights(FbxMesh* Mesh, TArray<FSkinnedVertex>& Vertices, const TArray<FBoneInfo>& Bones, const TMap<int32, TArray<int32>>& CP2Verts);
    static void ParseStaticMesh(FbxMesh* Mesh, OBJ::FStaticMeshRenderData& OutData);
};
