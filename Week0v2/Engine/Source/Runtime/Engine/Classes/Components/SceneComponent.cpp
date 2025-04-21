#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Math/JungleMath.h"
#include "UObject/ObjectFactory.h"
#include "ActorComponentInfo.h"

USceneComponent::USceneComponent() :RelativeLocation(FVector(0.f, 0.f, 0.f)), RelativeRotation(FVector(0.f, 0.f, 0.f)), RelativeScale(FVector(1.f, 1.f, 1.f))
{
}
USceneComponent::USceneComponent(const USceneComponent& Other)
    : UActorComponent(Other),
      AttachParent(nullptr), // 복제 시 복원
      RelativeLocation(Other.RelativeLocation),
      RelativeRotation(Other.RelativeRotation),
      RelativeScale(Other.RelativeScale),
      AABB(Other.AABB)
{
}
USceneComponent::~USceneComponent()
{
}
void USceneComponent::InitializeComponent()
{
   Super::InitializeComponent();
}

void USceneComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
}


int USceneComponent::CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance)
{
    int nIntersections = 0;
    return nIntersections;
}

FVector USceneComponent::GetForwardVector() const
{
    FVector Forward = FVector(1.f, 0.f, 0.0f);
    Forward = JungleMath::FVectorRotate(Forward, GetComponentRotation());
    return Forward;
}

FVector USceneComponent::GetRightVector()
{
    FVector Right = FVector(0.f, 1.f, 0.0f);
    Right = JungleMath::FVectorRotate(Right, GetComponentRotation());
    return Right;
}

FVector USceneComponent::GetUpVector()
{
    FVector Up = FVector(0.f, 0.f, 1.0f);
    Up = JungleMath::FVectorRotate(Up, GetComponentRotation());
    return Up;
}

void USceneComponent::AddLocation(FVector _added)
{
	RelativeLocation = RelativeLocation + _added;
}

void USceneComponent::AddRotation(FVector _added)
{
	RelativeRotation = RelativeRotation + _added;
}

void USceneComponent::AddScale(FVector _added)
{
	RelativeScale = RelativeScale + _added;
}

void USceneComponent::AttachToComponent(USceneComponent* InParent)
{
    // 기존 부모와 연결을 끊기
    if (AttachParent)
    {
        AttachParent->AttachChildren.Remove(this);
    }

    // InParent도 nullptr이면 부모를 nullptr로 설정
    if (InParent == nullptr)
    {
        AttachParent = nullptr;
        return;
    }


    // 새로운 부모 설정
    AttachParent = InParent;

    // 부모의 자식 리스트에 추가
    if (!InParent->AttachChildren.Contains(this))
    {
        InParent->AttachChildren.Add(this);
    }
}

FVector USceneComponent::GetComponentLocation() const
{
    if (AttachParent)
    {
        return AttachParent->GetComponentLocation() + RelativeLocation;
    }
    return RelativeLocation;
}

FRotator USceneComponent::GetComponentRotation() const
{
    if (AttachParent)
    {
        return AttachParent->GetComponentRotation().ToQuaternion() * RelativeRotation.ToQuaternion();
    }
    return RelativeRotation;
}

FVector USceneComponent::GetComponentScale() const
{
    if (AttachParent)
    {
        return AttachParent->GetComponentScale() * RelativeScale;
    }
    return RelativeScale;
}

FMatrix USceneComponent::GetScaleMatrix() const
{
    FMatrix ScaleMat = FMatrix::GetScaleMatrix(RelativeScale);
    if (AttachParent)
    {
        FMatrix ParentScaleMat = AttachParent->GetScaleMatrix();
        ScaleMat = ScaleMat * ParentScaleMat;
    }
    return ScaleMat;
}

FMatrix USceneComponent::GetRotationMatrix() const
{
    FMatrix RotationMat = FMatrix::GetRotationMatrix(RelativeRotation);
    if (AttachParent)
    {
        FMatrix ParentRotationMat = AttachParent->GetRotationMatrix();
        RotationMat = RotationMat * ParentRotationMat;
    }
    return RotationMat;
}

FMatrix USceneComponent::GetTranslationMatrix() const
{
    FMatrix TranslationMat = FMatrix::GetTranslationMatrix(RelativeLocation);
    if (AttachParent)
    {
        FMatrix ParentTranslationMat = AttachParent->GetTranslationMatrix();
        TranslationMat = TranslationMat * ParentTranslationMat;
    }
    return TranslationMat;
}

FMatrix USceneComponent::GetWorldMatrix() const
{
    FMatrix ScaleMat = FMatrix::GetScaleMatrix(RelativeScale);
    FMatrix RotationMat = FMatrix::GetRotationMatrix(RelativeRotation.ToQuaternion());
    FMatrix TranslationMat = FMatrix::GetTranslationMatrix(RelativeLocation);

    FMatrix RTMat = RotationMat * TranslationMat;
    if (AttachParent)
    {
        FMatrix ParentScaleMat = AttachParent->GetScaleMatrix();
        FMatrix ParentRotationMat = AttachParent->GetRotationMatrix();
        FMatrix ParentTranslationMat = AttachParent->GetTranslationMatrix();
        
        ScaleMat = ScaleMat * ParentScaleMat;
        FMatrix ParentRTMat = ParentRotationMat * ParentTranslationMat;
        RTMat = RTMat * ParentRTMat;
    }
    return ScaleMat * RTMat;
}
//
// FMatrix USceneComponent::GetComponentTransform() const
// {
//     if (AttachParent)
//     {
//         return GetRelativeTransform() * AttachParent->GetComponentTransform();
//     }
//     else
//     {
//         return GetRelativeTransform();
//     }
// }
//
// FMatrix USceneComponent::GetComponentTranslateMatrix() const
// {
//     FMatrix LocMat = FMatrix::CreateTranslationMatrix(RelativeLocation);
//     if (AttachParent)
//     {
//         FMatrix ParentLocMat = AttachParent->GetComponentTranslateMatrix();
//         LocMat = LocMat * ParentLocMat;
//     }
//     return LocMat;
// }
//
// FMatrix USceneComponent::GetComponentRotationMatrix() const
// {
//     FMatrix RotMat = FMatrix::CreateRotationMatrix(RelativeRotation.Roll, RelativeRotation.Pitch, RelativeRotation.Yaw);
//     if (AttachParent)
//     {
//         FMatrix ParentRotMat = AttachParent->GetComponentRotationMatrix();
//         RotMat = RotMat * ParentRotMat;
//     }
//     return RotMat;
// }
//
// FMatrix USceneComponent::GetComponentScaleMatrix() const
// {
//     FMatrix ScaleMat = FMatrix::CreateScaleMatrix(RelativeScale3D.x, RelativeScale3D.y, RelativeScale3D.z);
//     if (AttachParent)
//     {
//         FMatrix ParentScaleMat = AttachParent->GetComponentScaleMatrix();
//         ScaleMat = ScaleMat * ParentScaleMat;
//     }
//     return ScaleMat;
// }

void USceneComponent::SetupAttachment(USceneComponent* InParent)
{
    if (
        InParent != AttachParent                                  // 설정하려는 Parent가 기존의 Parent와 다르거나
        && InParent != this                                       // InParent가 본인이 아니고
        && InParent != nullptr                                    // InParent가 유효한 포인터 이며
        && (
            AttachParent == nullptr                               // AttachParent도 유효하며
            || !AttachParent->AttachChildren.Contains(this)  // 이미 AttachParent의 자식이 아닌 경우
        ) 
    ) {
        SetAttachParent(InParent);
        InParent->AttachChildren.AddUnique(this);
    }
}

void USceneComponent::DetachFromParent()
{
    if (AttachParent)
    {
        AttachParent->AttachChildren.Remove(this);
        AttachParent = nullptr;
    }
}


USceneComponent* USceneComponent::GetAttachParent() const
{
    return AttachParent;
}

void USceneComponent::SetAttachParent(USceneComponent* InParent)
{
    AttachParent = InParent;
}

UObject* USceneComponent::Duplicate() const
{
    USceneComponent* NewComp = FObjectFactory::ConstructObjectFrom<USceneComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();
    return NewComp;
}

void USceneComponent::DuplicateSubObjects(const UObject* Source)
{
    UActorComponent::DuplicateSubObjects(Source);
    // AttachParent는 AActor::DuplicateSubObjects에서 복원
}

void USceneComponent::PostDuplicate() {}

std::shared_ptr<FActorComponentInfo> USceneComponent::GetActorComponentInfo()
{
    std::shared_ptr<FSceneComponentInfo> Info = std::make_shared<FSceneComponentInfo>();
    Super::GetActorComponentInfo()->Copy(*Info);

    Info->InfoType = GetClass()->GetName();
    Info->RelativeLocation = RelativeLocation;
    Info->RelativeRotation = RelativeRotation;
    Info->RelativeScale3D = RelativeScale;

    // !TODO : AttachedParent
    return Info;
}

void USceneComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);
    const FSceneComponentInfo& SceneInfo = static_cast<const FSceneComponentInfo&>(Info);
    RelativeLocation = SceneInfo.RelativeLocation;
    RelativeRotation = SceneInfo.RelativeRotation;
    RelativeScale = SceneInfo.RelativeScale3D;
}
