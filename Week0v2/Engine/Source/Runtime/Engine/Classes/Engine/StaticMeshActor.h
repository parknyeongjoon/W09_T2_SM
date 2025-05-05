#pragma once
#include "GameFramework/Actor.h"


class UStaticMeshComponent;

class AStaticMeshActor : public AActor
{
    DECLARE_CLASS(AStaticMeshActor, AActor)

public:
    AStaticMeshActor();
    AStaticMeshActor(const AStaticMeshActor& Other);
    
    void BeginPlay() override;
    void Tick(float DeltaTime) override;
    void Destroyed() override;

    UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }

private:
    UStaticMeshComponent* StaticMeshComponent = nullptr;
};
