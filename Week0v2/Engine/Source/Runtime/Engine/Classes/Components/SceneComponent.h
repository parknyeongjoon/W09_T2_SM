#pragma once
#include "ActorComponent.h"
#include "Math/Quat.h"
#include "UObject/ObjectMacros.h"
#include "ActorComponentInfo.h"
#include "Math/Rotator.h"
#include "Misc/Guid.h"
#include <sol\sol.hpp>

class USceneComponent;

struct FSceneComponentInfo : public FActorComponentInfo
{
    DECLARE_ACTORCOMPONENT_INFO(FSceneComponentInfo);

    FVector RelativeLocation;
    FRotator RelativeRotation;
    FVector RelativeScale3D;
    FBoundingBox AABB;

    
    FGuid ParentComponentID;
    

    FSceneComponentInfo()
        : FActorComponentInfo()
        , RelativeLocation(FVector::ZeroVector)
        , RelativeRotation(FVector::ZeroVector)
        , RelativeScale3D(FVector::OneVector)
        , AABB(FBoundingBox(FVector::ZeroVector, FVector::ZeroVector))
    {
        InfoType = TEXT("FSceneComponentInfo");
        //ComponentClass = TEXT("USceneComponent");
    }
    

    virtual void Serialize(FArchive& ar) const override
    {
        FActorComponentInfo::Serialize(ar);
        ar << RelativeLocation << RelativeRotation << RelativeScale3D << AABB;
        ar << ParentComponentID.A << ParentComponentID.B << ParentComponentID.C << ParentComponentID.D;
    }

    virtual void Deserialize(FArchive& ar) override
    {
        FActorComponentInfo::Deserialize(ar);
        ar >> RelativeLocation >> RelativeRotation >> RelativeScale3D >> AABB;
        ar >> ParentComponentID.A >> ParentComponentID.B >> ParentComponentID.C >> ParentComponentID.D;
    }
};

class USceneComponent : public UActorComponent
{
    DECLARE_CLASS(USceneComponent, UActorComponent)
    friend class FSceneComponentInfo;

public:
    USceneComponent();
    USceneComponent(const USceneComponent& Other);
    virtual ~USceneComponent() override;

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;
    void DestroyComponent() override;
    virtual int CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance);
    virtual FVector GetForwardVector() const;
    virtual FVector GetRightVector() const;
    virtual FVector GetUpVector() const;
    void AddLocation(FVector _added);
    void AddRotation(FRotator _added);
    void AddScale(FVector _added);
    FVector GetWorldLocation() const;
    FRotator GetWorldRotation() const;
    FVector GetWorldScale() const;
    FMatrix GetScaleMatrix() const;
    FMatrix GetRotationMatrix() const;
    FMatrix GetTranslationMatrix() const;
    FMatrix GetWorldMatrix() const;

    void SetWorldLocation(const FVector& NewWorldLocation);
    FMatrix GetWorldToLocalMatrix() const;

    FBoundingBox GetBoundingBox() const { return AABB; }
    void SetBoundingBox(const FBoundingBox& InAABB) { AABB = InAABB; }
protected:
    FVector RelativeLocation;
    FRotator RelativeRotation;
    FVector RelativeScale;

    USceneComponent* AttachParent = nullptr;
    TArray<USceneComponent*> AttachChildren;

    FBoundingBox AABB;
public:
    UPROPERTY(FVector, RelativeLocation)
    UPROPERTY(FRotator, RelativeRotation)
    UPROPERTY(FVector, RelativeScale)
    
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
    std::unique_ptr<FActorComponentInfo> GetComponentInfo() override;
    virtual void SaveComponentInfo(FActorComponentInfo& OutInfo) override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info) override;

private:
    class UTextUUID* uuidText = nullptr;
private:
    FGuid PendingAttachParentID; // 로드 후 링크를 위해 임시 저장
public:
    FGuid GetPendingAttachParentID() const { return PendingAttachParentID; }
};
