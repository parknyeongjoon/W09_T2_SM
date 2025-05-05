#include "StaticMeshActor.h"

#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"


AStaticMeshActor::AStaticMeshActor()
{
    StaticMeshComponent = AddComponent<UStaticMeshComponent>(EComponentOrigin::Constructor);
    RootComponent = StaticMeshComponent;
}

AStaticMeshActor::AStaticMeshActor(const AStaticMeshActor& Other)
    : Super(Other)
{
}

void AStaticMeshActor::BeginPlay()
{
    Super::BeginPlay();
}

void AStaticMeshActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

void AStaticMeshActor::Destroyed()
{
    Super::Destroyed();
}

