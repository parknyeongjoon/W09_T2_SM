#include "DirectionalLightComponent.h"
#include "UObject/ObjectFactory.h"
#include "CoreUObject/UObject/Casts.h"
#include "EditorEngine.h"
#include "Math/JungleMath.h"

UDirectionalLightComponent::UDirectionalLightComponent()
{
    ShadowResource = FShadowResourceFactory::CreateShadowResource(GEngine->graphicDevice.Device, ELightType::DirectionalLight);
}

UDirectionalLightComponent::UDirectionalLightComponent(const UDirectionalLightComponent& Other)
    : Super(Other)
    , Direction(Other.Direction)
{
}

//void UDirectionalLightComponent::SetDirection(FVector _newDir)
//{
//    //잘 안됨
//    FVector Axis = Direction.Cross(_newDir).Normalize();
//    float Angle = acosf(Direction.Normalize().Dot(_newDir.Normalize()));
//    GetOwner()->GetRootComponent()->SetRelativeQuat(FQuat::FromAxisAngle(Axis, Angle));
//    Direction = _newDir;
//}
const float SCENE_RADIUS = 100.0f;
FMatrix UDirectionalLightComponent::GetViewMatrix() const
{
    // 광원 위치 결정 (씬의 중심에서 반대 방향으로)
    FVector sceneCenter = FVector(0,0,0); // TODO: Scene Center 넣기
    FVector lightPos = sceneCenter - GetForwardVector() * SCENE_RADIUS;
    // 광원 뷰 행렬 계산
    FVector upVector = FVector(0.0f, 0.0f, 1.0f);
    if (abs(GetForwardVector().Dot(upVector) > 0.9f))
    {
        upVector = FVector(0.0f, 0.0f, 1.0f);
    }
    
    FMatrix lightView = JungleMath::CreateViewMatrix(
        lightPos,
        sceneCenter,
        upVector);

    return lightView;
}

FMatrix UDirectionalLightComponent::GetProjectionMatrix() const
{
    // 직교 투영 행렬 계산 (방향광은 직교 투영 사용)
    return JungleMath::CreateOrthoProjectionMatrix(SCENE_RADIUS * 2,SCENE_RADIUS * 2, 0.1f, 4 * SCENE_RADIUS);
}

UObject* UDirectionalLightComponent::Duplicate() const
{
    UDirectionalLightComponent* NewComp = FObjectFactory::ConstructObjectFrom<UDirectionalLightComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();

    return NewComp;
}

void UDirectionalLightComponent::DuplicateSubObjects(const UObject* Source)
{
    Super::DuplicateSubObjects(Source);
    // 여기서 복사할 것? 방향뿐임
    UDirectionalLightComponent* SourceComp = Cast<UDirectionalLightComponent>(Source);
    SourceComp->Direction = Direction;
}

void UDirectionalLightComponent::PostDuplicate()
{
}

std::shared_ptr<FActorComponentInfo> UDirectionalLightComponent::GetActorComponentInfo()
{
    std::shared_ptr<FDirectionalLightComponentInfo> Info = std::make_shared<FDirectionalLightComponentInfo>();
    Super::GetActorComponentInfo()->Copy(*Info);
    Info->Direction = Direction;
    return Info;
}

void UDirectionalLightComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);
    const FDirectionalLightComponentInfo& DirectionalLightInfo = static_cast<const FDirectionalLightComponentInfo&>(Info);
    Direction = DirectionalLightInfo.Direction;
}
