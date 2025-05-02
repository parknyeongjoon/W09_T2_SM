#include "HitCameraShake.h"
#include "Math/Quat.h"
UHitCameraShake::UHitCameraShake()
{
    Duration = 0.25f;
    BlendInTime = 0.f;
    BlendOutTime = 0.1f;
}

UHitCameraShake::~UHitCameraShake()
{
}

void UHitCameraShake::UpdateShake(float DeltaTime, FVector& OutLoc, FRotator& OutRot, float& OutFOV)
{
    ElapsedTime += DeltaTime;
    float Alpha = FMath::Clamp(ElapsedTime / Duration, 0.f, 1.f);
    float W = GetBlendWeight();
    float Curve = FMath::InterpEaseOut(1.0f, 0.0f, Alpha, 2.5f);

    // 1. 방향 계산
    FVector Dir = (ImpactDirection - CameraLocation).Normalize();

    // 2. 카메라 기준으로 변환
    FQuat CameraQuat = CameraRotation.ToQuaternion();
    FQuat CameraQuatInv = FQuat(CameraQuat.w, -CameraQuat.x, -CameraQuat.y, -CameraQuat.z);
    FVector LocalDir = CameraQuatInv.RotateVector(Dir);

    // 3. 성분에 비례한 회전
    float TargetPitch = -LocalDir.X * RotationScale; // 앞에서 맞으면 아래로 밀림 (Pitch 증가)
    float TargetYaw = LocalDir.Y * RotationScale; // 왼쪽에서 맞으면 오른쪽으로 흔들림 (Yaw 증가)

    float CurrentPitch = TargetPitch * Curve * W;
    float CurrentYaw = TargetYaw * Curve * W;

    OutRot.Pitch = CurrentPitch - LastPitch;
    OutRot.Yaw = CurrentYaw - LastYaw;

    LastPitch = CurrentPitch;
    LastYaw = CurrentYaw;

    OutLoc = FVector::ZeroVector;
    OutFOV = 0.f;
}
