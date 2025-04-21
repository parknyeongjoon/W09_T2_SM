#include "LightComponentBase.h"

#include "Math/JungleMath.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "UObject/ObjectFactory.h"
#include "CoreUObject/UObject/Casts.h"
#include "GameFramework/Actor.h"

ULightComponentBase::ULightComponentBase()
{
    // FString name = "SpotLight";
    // SetName(name);
    LightColor = { 1,1,1,1 };
}

ULightComponentBase::ULightComponentBase(const ULightComponentBase& Other)
    : Super(Other)
    , LightColor(Other.LightColor)
    , Intensity(Other.Intensity)
{
}

ULightComponentBase::~ULightComponentBase()
{
    // remove shadow resource
    if (ShadowResource)
    {
        delete ShadowResource;
        ShadowResource = nullptr;
    }
}

void ULightComponentBase::SetColor(FVector4 newColor)
{
    LightColor = newColor;
}

FVector4 ULightComponentBase::GetColor() const
{
    return LightColor;
}

UObject* ULightComponentBase::Duplicate() const
{
    ULightComponentBase* NewComp = FObjectFactory::ConstructObjectFrom<ULightComponentBase>(this);
    //NewComp->InitializeLight();
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();
    return NewComp;
}

void ULightComponentBase::DuplicateSubObjects(const UObject* Source)
{
    // 복사할 것?
    Super::DuplicateSubObjects(Source);
}

void ULightComponentBase::PostDuplicate()
{
}

std::shared_ptr<FActorComponentInfo> ULightComponentBase::GetActorComponentInfo()
{
    std::shared_ptr<FLightComponentBaseInfo> Info = std::make_shared<FLightComponentBaseInfo>();
    Super::GetActorComponentInfo()->Copy(*Info);

    Info->Color = LightColor;
    Info->Intensity = Intensity;

    return Info;
}

void ULightComponentBase::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);
    const FLightComponentBaseInfo& LightInfo = static_cast<const FLightComponentBaseInfo&>(Info);
    LightColor = LightInfo.Color;
    Intensity = LightInfo.Intensity;
}

void ULightComponentBase::InitializeLight()
{
    AABB.max = { 1.f,1.f,0.1f };
    AABB.min = { -1.f,-1.f,-0.1f };
}

void ULightComponentBase::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);


}

int ULightComponentBase::CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance)
{
    bool res =AABB.Intersect(rayOrigin, rayDirection, pfNearHitDistance);
    return res;
}

void ULightComponentBase::InitializeComponent()
{

}

void ULightComponentBase::BeginPlay()
{
}

void ULightComponentBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    
}


