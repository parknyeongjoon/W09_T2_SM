#include "ProjectileMovementComponent.h"

#include "Components/PrimitiveComponents/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UProjectileMovementComponent::UProjectileMovementComponent()
{
    bUpdateOnlyIfRendered = false;
    bInitialVelocityInLocalSpace = true;

    Velocity = FVector(1.0f, 0.0f, 0.0f);

    ProjectileGravityScale = 1.0f;
    bWantsInitializeComponent = true;
}

UProjectileMovementComponent::UProjectileMovementComponent(const UProjectileMovementComponent& Other)
    : UMovementComponent(Other)
    , InitialSpeed(Other.InitialSpeed)
    , MaxSpeed(Other.MaxSpeed)
    , bRotationFollowsVelocity(Other.bRotationFollowsVelocity)
    , bInitialVelocityInLocalSpace(Other.bInitialVelocityInLocalSpace)
    , ProjectileGravityScale(Other.ProjectileGravityScale)
{
}


void UProjectileMovementComponent::InitializeComponent()
{
    UMovementComponent::InitializeComponent();

    if (Velocity.MagnitudeSquared() > 0.f)
    {
        if (InitialSpeed > 0.f)
        {
            Velocity = Velocity.Normalize() * InitialSpeed;
        }

        if (bInitialVelocityInLocalSpace)
        {
            // SetVelocityInLocalSpace(Velocity);
        }

        if (bRotationFollowsVelocity)
        {
            if (UpdatedComponent)
            {
                UpdatedComponent->SetRelativeRotation(Velocity); // Warning: Euler rotation not working. need FRotator
            }
        }

        UpdateComponentVelocity();
    }
}

void UProjectileMovementComponent::TickComponent(float DeltaTime)
{
    if (GetWorld()->WorldType == EWorldType::Editor) return; // Todo: change code.
    
    AActor* ActorOwner = UpdatedComponent->GetOwner();

    if (!ActorOwner)
    {
        return;
    }

    float RemainingTime = DeltaTime;
    int32 Iterations = 0;
    bool bSliding = false; // Not use
    
    while (RemainingTime > 0.f && (Iterations < 8) && !ActorOwner->IsActorBeingDestroyed() && UpdatedComponent)
    {
        Iterations++;

        float TimeTick = (!ShouldApplyGravity() || RemainingTime < 0.05f) ? RemainingTime : FMath::Min(0.05f, RemainingTime * 0.5f);
        RemainingTime -= TimeTick;

        const FVector OldVelocity = Velocity;
        const bool bApplyGravity = bSliding;
        FVector MoveDelta = ComputeMoveDelta(Velocity, TimeTick, bApplyGravity);

        const FVector TmpVelocity = Velocity;
        MoveComponent(MoveDelta);

        if (ActorOwner->IsActorBeingDestroyed() || !UpdatedComponent)
        {
            return;
        }

        {
            // 이동 전과 후가 속도가 같다면 새롭게 계산
            // 이는 물리적 손실이나 반작용을 반영하기 위함.
            // 현재는 물리 엔진이 적용되어 있지 않음.
            if (Velocity == TmpVelocity)
            {
                Velocity = CalculateVelocity(OldVelocity, TimeTick);
            }
        }
    }

    UpdateComponentVelocity();
}

void UProjectileMovementComponent::SetVelocityInLocalSpace(FVector NewVelocity)
{
    if (UpdatedComponent)
    {
        Velocity = UpdatedComponent->ComponentVelocity;
    }
}

FVector UProjectileMovementComponent::CalculateVelocity(FVector OldVelocity, float DeltaTime)
{
    FVector NewVelocity = OldVelocity;

    /**
     * Todo.
     * 제대로 된 중력 적용 X
     * 상수 계수로 적절히 조절하는 코드
     * 나중에 물리엔진이 추가되면 수정 필요
     */
    constexpr float DragCoefficient = 0.1f; // 적절한 계수로 조절
    NewVelocity = NewVelocity * (1.0f - DragCoefficient * DeltaTime);
    
    if (ShouldApplyGravity())
    {
        NewVelocity.Z += GetEffectiveGravityZ() * DeltaTime;
    }
    
    return LimitVelocity(NewVelocity);
}

UObject* UProjectileMovementComponent::Duplicate() const
{
    UProjectileMovementComponent* NewComp = FObjectFactory::ConstructObjectFrom<UProjectileMovementComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();
    return NewComp;
}

void UProjectileMovementComponent::DuplicateSubObjects(const UObject* Source)
{
    UMovementComponent::DuplicateSubObjects(Source);
}

void UProjectileMovementComponent::PostDuplicate()
{
    UMovementComponent::PostDuplicate();
}

std::unique_ptr<FActorComponentInfo> UProjectileMovementComponent::GetComponentInfo()
{
    auto Info = std::make_unique<FProjectileMovementComponentInfo>();
    SaveComponentInfo(*Info);
    
    return Info;
}


void UProjectileMovementComponent::SaveComponentInfo(FActorComponentInfo& OutInfo)
{
    FProjectileMovementComponentInfo* Info = static_cast<FProjectileMovementComponentInfo*>(&OutInfo);
    Super::SaveComponentInfo(*Info);
    Info->InitialSpeed = InitialSpeed;
    Info->MaxSpeed = MaxSpeed;
    Info->bRotationFollowsVelocity = bRotationFollowsVelocity;
    Info->bInitialVelocityInLocalSpace = bInitialVelocityInLocalSpace;
    Info->ProjectileGravityScale = ProjectileGravityScale;
}

void UProjectileMovementComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);
    const FProjectileMovementComponentInfo& ProjectileMovementInfo = static_cast<const FProjectileMovementComponentInfo&>(Info);
    InitialSpeed = ProjectileMovementInfo.InitialSpeed;
    MaxSpeed = ProjectileMovementInfo.MaxSpeed;
    bRotationFollowsVelocity = ProjectileMovementInfo.bRotationFollowsVelocity;
    bInitialVelocityInLocalSpace = ProjectileMovementInfo.bInitialVelocityInLocalSpace;
    ProjectileGravityScale = ProjectileMovementInfo.ProjectileGravityScale;
}

FVector UProjectileMovementComponent::LimitVelocity(FVector NewVelocity) const
{
    if (GetMaxSpeed() > 0.f)
    {
        NewVelocity = NewVelocity.ClampMaxSize(GetModifiedMaxSpeed());
    }
    
    return NewVelocity;
}

FVector UProjectileMovementComponent::ComputeMoveDelta(const FVector& InVelocity, float DeltaTime, bool bApplyGravity) const
{
    // x = x0 + v * t;
    FVector Delta = InVelocity * DeltaTime;

    // y = y0 + v * t + 1/2 * a * t^2;
    if (bApplyGravity)
    {
        Delta.Z += 0.5f * GetEffectiveGravityZ() * FMath::Square(DeltaTime);
    }
    
    return Delta;
}

float UProjectileMovementComponent::GetEffectiveGravityZ() const
{
    return GetGravityZ() * ProjectileGravityScale;
}


