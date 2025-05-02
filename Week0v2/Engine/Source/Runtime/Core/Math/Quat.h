#pragma once
#include "Matrix.h"
#include "Serialization/Archive.h"
// 쿼터니언
struct FQuat
{
	float w, x, y, z;

	// 기본 생성자
	FQuat() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}

	// FQuat 생성자 추가: 회전 축과 각도를 받아서 FQuat 생성
	FQuat(const FVector& Axis, float Angle)
	{
		float HalfAngle = Angle * 0.5f;
		float SinHalfAngle = sinf(HalfAngle);
		float CosHalfAngle = cosf(HalfAngle);

		x = Axis.X * SinHalfAngle;
		y = Axis.Y * SinHalfAngle;
		z = Axis.Z * SinHalfAngle;
		w = CosHalfAngle;
	}

	// w, x, y, z 값으로 초기화
	FQuat(float InW, float InX, float InY, float InZ) : w(InW), x(InX), y(InY), z(InZ) {}

	// 쿼터니언의 곱셈 연산 (회전 결합)
	FQuat operator*(const FQuat& Other) const
	{
		return FQuat(
			w * Other.w - x * Other.x - y * Other.y - z * Other.z,
			w * Other.x + x * Other.w + y * Other.z - z * Other.y,
			w * Other.y - x * Other.z + y * Other.w + z * Other.x,
			w * Other.z + x * Other.y - y * Other.x + z * Other.w
		);
	}

	// (쿼터니언) 벡터 회전
	FVector RotateVector(const FVector& Vec) const
	{
		// 벡터를 쿼터니언으로 변환
		FQuat vecQuat(0.0f, Vec.X, Vec.Y, Vec.Z);
		// 회전 적용 (q * vec * q^-1)
		FQuat conjugate = FQuat(w, -x, -y, -z);  // 쿼터니언의 켤레
		FQuat result = *this * vecQuat * conjugate;

		return FVector(result.x, result.y, result.z); // 회전된 벡터 반환
	}

	// 단위 쿼터니언 여부 확인
	bool IsNormalized() const
	{
		return fabs(w * w + x * x + y * y + z * z - 1.0f) < 1e-6f;
	}

	// 쿼터니언 정규화 (단위 쿼터니언으로 만듬)
	FQuat Normalize() const
	{
		float magnitude = sqrtf(w * w + x * x + y * y + z * z);
		return FQuat(w / magnitude, x / magnitude, y / magnitude, z / magnitude);
	}

    static FQuat Identity()
    {
        return FQuat(1.0f, 1.0f, 1.0f, 1.0f);
    }
	// 회전 각도와 축으로부터 쿼터니언 생성 (axis-angle 방식)
	static FQuat FromAxisAngle(const FVector& Axis, float Angle)
	{
		float halfAngle = Angle * 0.5f;
		float sinHalfAngle = sinf(halfAngle);
		return FQuat(cosf(halfAngle), Axis.X * sinHalfAngle, Axis.Y * sinHalfAngle, Axis.Z * sinHalfAngle);
	}

	static FQuat CreateRotation(float roll, float pitch, float yaw)
	{
		// 각도를 라디안으로 변환
		float radRoll = roll * (3.14159265359f / 180.0f);
		float radPitch = pitch * (3.14159265359f / 180.0f);
		float radYaw = yaw * (3.14159265359f / 180.0f);

		// 각 축에 대한 회전 쿼터니언 계산
		FQuat qRoll = FQuat(FVector(1.0f, 0.0f, 0.0f), radRoll);  // X축 회전
		FQuat qPitch = FQuat(FVector(0.0f, 1.0f, 0.0f), radPitch);  // Y축 회전
		FQuat qYaw = FQuat(FVector(0.0f, 0.0f, 1.0f), radYaw);  // Z축 회전

		// 회전 순서대로 쿼터니언 결합 (Y -> X -> Z)
		return qRoll * qPitch * qYaw;
	}
	// 쿼터니언을 회전 행렬로 변환
	FMatrix ToMatrix() const
	{
		FMatrix RotationMatrix;
		RotationMatrix.M[0][0] = 1.0f - 2.0f * (y * y + z * z);
		RotationMatrix.M[0][1] = 2.0f * (x * y - w * z);
		RotationMatrix.M[0][2] = 2.0f * (x * z + w * y);
		RotationMatrix.M[0][3] = 0.0f;


		RotationMatrix.M[1][0] = 2.0f * (x * y + w * z);
		RotationMatrix.M[1][1] = 1.0f - 2.0f * (x * x + z * z);
		RotationMatrix.M[1][2] = 2.0f * (y * z - w * x);
		RotationMatrix.M[1][3] = 0.0f;

		RotationMatrix.M[2][0] = 2.0f * (x * z - w * y);
		RotationMatrix.M[2][1] = 2.0f * (y * z + w * x);
		RotationMatrix.M[2][2] = 1.0f - 2.0f * (x * x + y * y);
		RotationMatrix.M[2][3] = 0.0f;

		RotationMatrix.M[3][0] = RotationMatrix.M[3][1] = RotationMatrix.M[3][2] = 0.0f;
		RotationMatrix.M[3][3] = 1.0f;

		return RotationMatrix;
	}

    void Serialize(FArchive& Ar) const
    {
        Ar << w << x << y << z;
    }
    void Deserialize(FArchive& Ar)
    {
        Ar >> w >> x >> y >> z;
    }
};
