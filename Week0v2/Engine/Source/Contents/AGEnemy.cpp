#include "AGEnemy.h"

#include <random>
#include "GameManager.h"
#include "Components/Material/Material.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include "Engine/FLoaderOBJ.h"

AGEnemy::AGEnemy()
{
    MeshComp = AddComponent<UStaticMeshComponent>(EComponentOrigin::Constructor);
    FManagerOBJ::CreateStaticMesh("Assets/Primitives/Capsule.obj");
    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Capsule.obj"));
    
    Capsule = AddComponent<UCapsuleShapeComponent>(EComponentOrigin::Constructor);

    UMaterial* Material = new UMaterial();
    FObjMaterialInfo MaterialInfo;
    Material->SetMaterialInfo(MaterialInfo);
    MeshComp->SetMaterial(0, Material);
    
    for (int i=0;i<3;i++)
    {
        HeartUI[i] = AddComponent<UBillboardComponent>(EComponentOrigin::Constructor);
        if (USceneComponent* ParentComponent = Cast<USceneComponent>(MeshComp))
        {
            HeartUI[i]->DetachFromParent();
            HeartUI[i]->SetupAttachment(ParentComponent);
        }
        HeartUI[i]->SetTexture(L"Assets/Texture/heartpixelart.png");
        HeartUI[i]->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f));
        HeartUI[i]->bOnlyForEditor = false;
        HeartUI[i]->SetRelativeLocation(FVector(i - 1, 0, 1.5f));
    }
    
    ChangeColor(FVector(0,0.7,0.4));
}

AGEnemy::AGEnemy(const AGEnemy& Other)
{
}

void AGEnemy::BeginPlay()
{
    Super::BeginPlay();
    OnHit.AddUObject(this, &AGEnemy::OnDamaged);
    OnDead.AddStatic(FGameManager::AddScore);
    OnDead.AddStatic(FGameManager::SpawnEnemy);
    OnBeginOverlap.AddUObject(this, &AGEnemy::OnCollision);
    static std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
    
    // 각 축별로 다른 분포 정의
    std::uniform_int_distribution<int> xDistribution(-10, 10);
    std::uniform_int_distribution<int> yDistribution(-10, 10);  // y축은 더 넓은 범위
    
    // 각 축에 대해 별도로 난수 생성
    Velocity = FVector(
        xDistribution(generator) * 0.001f, 
        yDistribution(generator) * 0.001f, 
        0
    );
}

void AGEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    Move();
    if (bIsDead)
    {
        DeadTimer += DeltaTime;
        if (DeadTimer > 1.0f)
        {
            Destroy();
        }
    }
}

void AGEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

UObject* AGEnemy::Duplicate() const
{
    AGEnemy* NewActor = FObjectFactory::ConstructObjectFrom<AGEnemy>(this);
    NewActor->DuplicateSubObjects(this);
    return NewActor;
}

void AGEnemy::DuplicateSubObjects(const UObject* Source) // TODO: Duplicate 고치기
{
    int heartIndex = 0;
    Super::DuplicateSubObjects(Source);
    for (auto& Comp : GetComponents())
    {
        if (UStaticMeshComponent* SMComp = Cast<UStaticMeshComponent>(Comp))
        {
            MeshComp = SMComp;
        }
        if (UBillboardComponent* HBComp = Cast<UBillboardComponent>(Comp))
        {
            HeartUI[heartIndex] = HBComp;
            HeartUI[heartIndex]->SetRelativeLocation(FVector(-1 + heartIndex, 0, 0));
            heartIndex++;
        }
        if (UCapsuleShapeComponent* CapComp = Cast<UCapsuleShapeComponent>(Comp))
        {
            Capsule = CapComp;
        }
    }
}

void AGEnemy::PostDuplicate()
{
    Super::PostDuplicate();
}

void AGEnemy::OnDamaged()
{
    health--;
    
    switch (health)
    {
    case 2:
        ChangeColor(FVector(1,1,0));
        HeartUI[2]->DestroyComponent();
        break;
    case 1:
        ChangeColor(FVector(1,0,0));
        HeartUI[1]->DestroyComponent();
        break;
    case 0:
        OnDead.Broadcast();
        ChangeColor(FVector(0.1,0.1,0.1));
        Velocity = FVector(0, 0, 0);
        bIsDead = true;
        break;
    }
}

void AGEnemy::ChangeColor(FVector NewColor) const
{
    MeshComp->GetMaterial(0)->SetDiffuse(NewColor);
}

void AGEnemy::Move()
{
    SetActorLocation(GetActorLocation() + Velocity);
}

void AGEnemy::OnCollision(const UPrimitiveComponent* Other)
{
    Velocity = FVector(-Velocity.X, -Velocity.Y,0);
    if (Cast<USphereShapeComponent>(Other))
    {
        OnHit.Broadcast();
    }
}
