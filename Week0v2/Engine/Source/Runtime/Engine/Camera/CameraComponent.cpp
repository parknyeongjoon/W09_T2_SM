#include "CameraComponent.h"

#include "EditorEngine.h"
#include "Math/JungleMath.h"
#include "Engine/World.h"
#include "UnrealEd/EditorViewportClient.h"
#include "LevelEditor/SLevelEditor.h"
#include "Math/JungleMath.h"

#include "Classes/GameFramework/Actor.h"
#include "Components/PrimitiveComponents/Physics/USphereShapeComponent.h"

UCameraComponent::UCameraComponent()
{
}

UCameraComponent::UCameraComponent(const UCameraComponent& Other)
    :Super(Other),
    FOV(Other.FOV), nearClip(Other.nearClip), farClip(Other.farClip)
{
}

UCameraComponent::~UCameraComponent()
{
}

void UCameraComponent::InitializeComponent()
{
	Super::InitializeComponent();
	//RelativeLocation = FVector(0.0f, 0.0f, 0.5f);
	FOV = 60.f;
}

void UCameraComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    Input();
    SetRelativeRotation(RelativeRotation);
}

void UCameraComponent::Input()
{
    return;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) // VK_RBUTTON은 마우스 오른쪽 버튼을 나타냄
    {
        if (!bRightMouseDown)
        {
            // 마우스 오른쪽 버튼을 처음 눌렀을 때, 마우스 위치 초기화
            bRightMouseDown = true;
            GetCursorPos(&lastMousePos);
        }
        else
        {
            // 마우스 이동량 계산
            POINT currentMousePos;
            GetCursorPos(&currentMousePos);

            // 마우스 이동 차이 계산
            int32 deltaX = currentMousePos.x - lastMousePos.x;
            int32 deltaY = currentMousePos.y - lastMousePos.y;

            // Yaw(좌우 회전) 및 Pitch(상하 회전) 값 변경
            RotateYaw(deltaX * 0.1f);  // X 이동에 따라 좌우 회전
            RotatePitch(deltaY * 0.1f);  // Y 이동에 따라 상하 회전

            // 새로운 마우스 위치 저장
            lastMousePos = currentMousePos;
        }
        if (GetAsyncKeyState('A') & 0x8000)
        {
            MoveRight(-1.f);
        }
        if (GetAsyncKeyState('D') & 0x8000)
        {
            MoveRight(1.f);
        }
        if (GetAsyncKeyState('W') & 0x8000)
        {
            MoveForward(1.f);
        }
        if (GetAsyncKeyState('S') & 0x8000)
        {
            MoveForward(-1.f);
        }
        if (GetAsyncKeyState('E') & 0x8000)
        {
            MoveUp(1.f);
        }
        if (GetAsyncKeyState('Q') & 0x8000)
        {
            MoveUp(-1.f);
        }
    }
    else
    {
        bRightMouseDown = false; // 마우스 오른쪽 버튼을 떼면 상태 초기화
    }

}

void UCameraComponent::MoveForward(float _Value)
{
    RelativeLocation = RelativeLocation + GetForwardVector() * GetEngine()->GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar() * _Value;
}

void UCameraComponent::MoveRight(float _Value)
{
    //FVector newRight = FVector(GetRightVector().x, GetRightVector().y, 0.0f);
    RelativeLocation = RelativeLocation + GetRightVector() * GetEngine()->GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar() * _Value;
}

void UCameraComponent::MoveUp(float _Value)
{
    RelativeLocation.Z += _Value * GetEngine()->GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar();
}

void UCameraComponent::RotateYaw(float _Value)
{
    RelativeRotation.Yaw += _Value * GetEngine()->GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar();
}

void UCameraComponent::RotatePitch(float _Value)
{

    RelativeRotation.Pitch += _Value;
    if (RelativeRotation.Pitch < -90.0f)
        RelativeRotation.Pitch = -90.0f;
    if (RelativeRotation.Pitch > 90.0f)
        RelativeRotation.Pitch = 90.0f;
}

UObject* UCameraComponent::Duplicate() const
{
    UCameraComponent* NewComp = FObjectFactory::ConstructObjectFrom<UCameraComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();

    return NewComp;
}

void UCameraComponent::DuplicateSubObjects(const UObject* Source)
{
    Super::DuplicateSubObjects(Source);
}

void UCameraComponent::PostDuplicate()
{
    Super::PostDuplicate();
}

FMatrix UCameraComponent::GetViewMatrix() const
{
    //FVector CameraPos = GetWorldLocation();
    FVector CameraPos = GetWorldMatrix().TransformPosition(GetRelativeLocation());
    FVector CameraForward = GetForwardVector();
    //FVector4 CameraForward4 = FMatrix::TransformVector(FVector4(1, 0, 0, 0), GetWorldMatrix());
    //FVector CameraForward = FVector(CameraForward4.x, CameraForward4.y, CameraForward4.z);
    FVector CameraUP = FVector(0, 0, 1);

    return JungleMath::CreateViewMatrix(CameraPos, CameraPos + CameraForward, CameraUP);
}

FMatrix UCameraComponent::GetProjectionMatrix() const
{
    float AspectRatio = GEngine->GetAspectRatio(GEngine->graphicDevice.SwapChain);
    return JungleMath::CreateProjectionMatrix(
        FMath::DegreesToRadians(FOV),
        AspectRatio,
        nearClip,
        farClip
    );
}
