#pragma once
#include "Components/PrimitiveComponents/MeshComponents/USkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

class ASkeletalMeshActor : public AActor
{
    DECLARE_CLASS(ASkeletalMeshActor, AActor)
    
public:
    ASkeletalMeshActor();
    
    USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }

private:
    USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
};
