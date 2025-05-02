#include "Rotator.h"

#include "Vector.h"
#include "Quat.h"
#include "Matrix.h"
#include "Misc/Parse.h"

const FRotator FRotator::ZeroRotator = FRotator(0.0f, 0.0f, 0.0f);

FRotator::FRotator(const FVector& InVector)
    : Pitch(FMath::RadiansToDegrees(InVector.Y)), Yaw(FMath::RadiansToDegrees(InVector.Z)), Roll(FMath::RadiansToDegrees(InVector.X))
{
}

FRotator::FRotator(const FQuat& InQuat)
{
    const float SingularityTest = InQuat.z * InQuat.x - InQuat.w * InQuat.y;
    const float YawY = 2.f * (InQuat.w * InQuat.z + InQuat.x * InQuat.y);
    const float YawX = (1.f - 2.f * (FMath::Square(InQuat.y) + FMath::Square(InQuat.z)));

    // reference 
    // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/

    // this value was found from experience, the above websites recommend different values
    // but that isn't the case for us, so I went through different testing, and finally found the case 
    // where both of world lives happily. 
    const float SINGULARITY_THRESHOLD = 0.4999995f;
    const float RAD_TO_DEG = (180.f / PI);

    if (SingularityTest < -SINGULARITY_THRESHOLD)
    {
        Pitch = -90.f;
        Yaw = (FMath::Atan2(YawY, YawX) * RAD_TO_DEG);
        Roll = (-Yaw - (2.f * atan2(InQuat.x, InQuat.w) * RAD_TO_DEG));
    }
    else if (SingularityTest > SINGULARITY_THRESHOLD)
    {
        Pitch = 90.f;
        Yaw = (FMath::Atan2(YawY, YawX) * RAD_TO_DEG);
        Roll = (Yaw - (2.f * FMath::Atan2(InQuat.x, InQuat.w) * RAD_TO_DEG));
    }
    else
    {
        Pitch = (asin(2.f * SingularityTest) * RAD_TO_DEG);
        Yaw = (FMath::Atan2(YawY, YawX) * RAD_TO_DEG);
        Roll = (FMath::Atan2(-2.f * (InQuat.w * InQuat.x + InQuat.y * InQuat.z),
            (1.f - 2.f * (FMath::Square(InQuat.x) + FMath::Square(InQuat.y)))) * RAD_TO_DEG);
    }
}

FRotator FRotator::operator+(const FRotator& Other) const
{
    return FRotator(Pitch + Other.Pitch, Yaw + Other.Yaw, Roll + Other.Roll);
}

FRotator& FRotator::operator+=(const FRotator& Other)
{
    Pitch += Other.Pitch; Yaw += Other.Yaw; Roll += Other.Roll;
    return *this;
}

FRotator FRotator::operator-(const FRotator& Other) const
{
    return { Pitch - Other.Pitch, Yaw - Other.Yaw, Roll - Other.Roll };
}

FRotator& FRotator::operator-=(const FRotator& Other)
{
    Pitch -= Other.Pitch; Yaw -= Other.Yaw; Roll -= Other.Roll;
    return *this;
}

FRotator FRotator::operator*(float Scalar) const
{
    return { Pitch * Scalar, Yaw * Scalar, Roll * Scalar };
}

FRotator& FRotator::operator*=(float Scalar)
{
    Pitch *= Scalar; Yaw *= Scalar; Roll *= Scalar;
    return *this;
}

FRotator FRotator::operator/(const FRotator& Other) const
{
    return { Pitch / Other.Pitch, Yaw / Other.Yaw, Roll / Other.Roll };
}

FRotator FRotator::operator/(float Scalar) const
{
    return { Pitch / Scalar, Yaw / Scalar, Roll / Scalar };
}

FRotator& FRotator::operator/=(float Scalar)
{
    Pitch /= Scalar; Yaw /= Scalar; Roll /= Scalar;
    return *this;
}

FRotator FRotator::operator-() const
{
    return { -Pitch, -Yaw, -Roll };
}

bool FRotator::operator==(const FRotator& Other) const
{
    return Pitch == Other.Pitch && Yaw == Other.Yaw && Roll == Other.Roll;
}

bool FRotator::operator!=(const FRotator& Other) const
{
    return Pitch != Other.Pitch || Yaw != Other.Yaw || Roll != Other.Roll;
}

bool FRotator::IsNearlyZero(float Tolerance) const
{
    return FMath::Abs(Pitch) <= Tolerance && FMath::Abs(Yaw) <= Tolerance && FMath::Abs(Roll) <= Tolerance;
}

bool FRotator::IsZero() const
{
    return Pitch == 0.0f && Yaw == 0.0f && Roll == 0.0f;
}

bool FRotator::Equals(const FRotator& Other, float Tolerance) const
{
    return FMath::Abs(Pitch - Other.Pitch) <= Tolerance && FMath::Abs(Yaw - Other.Yaw) <= Tolerance && FMath::Abs(Roll - Other.Roll) <= Tolerance;

}

FRotator FRotator::Add(float DeltaPitch, float DeltaYaw, float DeltaRoll) const
{
    return FRotator(Pitch + DeltaPitch, Yaw + DeltaYaw, Roll + DeltaRoll);
}

FRotator FRotator::FromQuaternion(const FQuat& InQuat) const
{
    return FRotator(InQuat);
}

FQuat FRotator::ToQuaternion() const
{
    float DegToRad = PI / 180.0f;
    float Div =  DegToRad / 2.0f;
    float SP, SY, SR;
    float CP, CY, CR;

    FMath::SinCos(&SP, &CP, Pitch * Div);
    FMath::SinCos(&SY, &CY, Yaw * Div);
    FMath::SinCos(&SR, &CR, Roll * Div);
	
    FQuat RotationQuat;
    RotationQuat.x = CR * SP * SY - SR * CP * CY;
    RotationQuat.y = -CR * SP * CY - SR * CP * SY;
    RotationQuat.z = CR * CP * SY - SR * SP * CY;
    RotationQuat.w = CR * CP * CY + SR * SP * SY;

    return RotationQuat;
}

FVector FRotator::ToVector() const
{
    return FVector(FMath::DegreesToRadians(Roll), FMath::DegreesToRadians(Pitch), FMath::DegreesToRadians(Yaw));
}

FMatrix FRotator::ToMatrix() const
{
    return FMatrix::GetRotationMatrix(*this);
}

float FRotator::Clamp(float Angle) const
{
    Angle = std::fmod(Angle, 360.0f);
    if (Angle < 0.0f)
    {
        Angle += 360.0f;
    }
    return Angle;
}

FRotator FRotator::GetNormalized() const
{
    return { FMath::UnwindDegrees(Pitch), FMath::UnwindDegrees(Yaw), FMath::UnwindDegrees(Roll) };
}

void FRotator::Normalize()
{
    Pitch = FMath::UnwindDegrees(Pitch);
    Yaw = FMath::UnwindDegrees(Yaw);
    Roll = FMath::UnwindDegrees(Roll);
}

FString FRotator::ToString() const
{
    return FString::Printf(TEXT("Pitch=%3.3f Yaw=%3.3f Roll=%3.3f"), Pitch, Yaw, Roll);
}

bool FRotator::InitFromString(const FString& InSourceString)
{
    Pitch = 0.0f;
    Yaw = 0.0f;
    Roll = 0.0f;

    const bool bSuccess = FParse::Value(*InSourceString, TEXT("Pitch="), Pitch) &&
        FParse::Value(*InSourceString, TEXT("Yaw="), Yaw) &&
        FParse::Value(*InSourceString, TEXT("Roll="), Roll);

    return bSuccess;
}


