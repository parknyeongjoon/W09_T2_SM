#pragma once
#include "ActorComponent.h"
#include "Math/Quat.h"
#include "UObject/ObjectMacros.h"
#include "ActorComponentInfo.h"
#include "Math/Rotator.h"

struct FSceneComponentInfo : public FActorComponentInfo
{
    DECLARE_ACTORCOMPONENT_INFO(FSceneComponentInfo);
    
    FVector RelativeLocation;
    FRotator RelativeRotation;
    FVector RelativeScale3D;
    FBoundingBox AABB;

    FSceneComponentInfo()
        : FActorComponentInfo()
        , RelativeLocation(FVector::ZeroVector)
        , RelativeRotation(FVector::ZeroVector)
        , RelativeScale3D(FVector::OneVector)
        , AABB(FBoundingBox(FVector::ZeroVector, FVector::ZeroVector))
    {
        InfoType = TEXT("FSceneComponentInfo");
        ComponentType = TEXT("USceneComponent");
    }

    virtual void Copy(FActorComponentInfo& Other) override
    {
        FActorComponentInfo::Copy(Other);
        FSceneComponentInfo& OtherScene = static_cast<FSceneComponentInfo&>(Other);
        OtherScene.RelativeLocation = RelativeLocation;
        OtherScene.RelativeRotation = RelativeRotation;
        OtherScene.RelativeScale3D = RelativeScale3D;
        OtherScene.AABB = AABB;
    }

    virtual void Serialize(FArchive& ar) const override
    {
        FActorComponentInfo::Serialize(ar);
        ar << RelativeLocation << RelativeRotation << RelativeScale3D << AABB;
    }

    virtual void Deserialize(FArchive& ar) override
    {
        FActorComponentInfo::Deserialize(ar);
        ar >> RelativeLocation >> RelativeRotation >> RelativeScale3D >> AABB;
    }
};

class USceneComponent : public UActorComponent
{
    DECLARE_CLASS(USceneComponent, UActorComponent)

public:
    USceneComponent();
    USceneComponent(const USceneComponent& Other);
    virtual ~USceneComponent() override;

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;
    virtual int CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance);
    virtual FVector GetForwardVector() const;
    virtual FVector GetRightVector();
    virtual FVector GetUpVector();
    void AddLocation(FVector _added);
    void AddRotation(FVector _added);
    void AddScale(FVector _added);
    FVector GetComponentLocation() const;
    FRotator GetComponentRotation() const;
    FVector GetComponentScale() const;
    FMatrix GetScaleMatrix() const;
    FMatrix GetRotationMatrix() const;
    FMatrix GetTranslationMatrix() const;
    FMatrix GetWorldMatrix() const;

    FBoundingBox GetBoundingBox() { return AABB; }
    void SetBoundingBox(const FBoundingBox& InAABB) { AABB = InAABB; }
protected:
    FVector RelativeLocation;
    FRotator RelativeRotation;
    FVector RelativeScale;

    USceneComponent* AttachParent = nullptr;
    TArray<USceneComponent*> AttachChildren;

    FBoundingBox AABB;
public:
    PROPERTY(FVector, RelativeLocation)
    PROPERTY(FRotator, RelativeRotation)
    PROPERTY(FVector, RelativeScale)
    
    void AttachToComponent(USceneComponent* InParent);

#define region
    FVector GetLocalScale() const { return RelativeScale; }
    FVector GetLocalLocation() const { return RelativeLocation; }
    
    void SetupAttachment(USceneComponent* InParent);
    void DetachFromParent();
#define endregion
public:
    USceneComponent* GetAttachParent() const;
    void SetAttachParent(USceneComponent* InParent);
    TArray<USceneComponent*> GetAttachChildren() const { return AttachChildren; }
    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate();

public:
    virtual bool MoveComponent(const FVector& Delta) { return false; }
    virtual std::shared_ptr<FActorComponentInfo> GetActorComponentInfo() override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info) override;

private:
    class UTextUUID* uuidText = nullptr;
public:
};
