#pragma once
#include "Vector.h"
#include "Vector4.h"

struct FRotator;
struct FQuat;

// 4x4 행렬 연산
struct FMatrix
{
    float M[4][4];
    static const FMatrix Identity;
    // 기본 연산자 오버로딩
    FMatrix operator+(const FMatrix& Other) const;
    FMatrix operator-(const FMatrix& Other) const;
    FMatrix operator*(const FMatrix& Other) const;
    FMatrix operator*(float Scalar) const;
    FMatrix operator/(float Scalar) const;
    float* operator[](int row);
    const float* operator[](int row) const;
	
    // 유틸리티 함수
    static FMatrix Transpose(const FMatrix& Mat);
    static float Determinant(const FMatrix& Mat);
    static FMatrix Inverse(const FMatrix& Mat);
    static FMatrix CreateRotationMatrix(float roll, float pitch, float yaw);
    static FMatrix CreateScaleMatrix(float scaleX, float scaleY, float scaleZ);
    static FVector TransformVector(const FVector& v, const FMatrix& m);
    static FVector4 TransformVector(const FVector4& v, const FMatrix& m);
    static FMatrix CreateTranslationMatrix(const FVector& position);

    static FMatrix GetScaleMatrix(const FVector& InScale);
    static FMatrix GetTranslationMatrix(const FVector& InPosition);
    static FMatrix GetRotationMatrix(const FRotator& InRotation);
    static FMatrix GetRotationMatrix(const FQuat& InRotation);
    
    FVector4 TransformFVector4(const FVector4& vector)
    {
        return FVector4(
            M[0][0] * vector.x + M[1][0] * vector.y + M[2][0] * vector.z + M[3][0] * vector.w,
            M[0][1] * vector.x + M[1][1] * vector.y + M[2][1] * vector.z + M[3][1] * vector.w,
            M[0][2] * vector.x + M[1][2] * vector.y + M[2][2] * vector.z + M[3][2] * vector.w,
            M[0][3] * vector.x + M[1][3] * vector.y + M[2][3] * vector.z + M[3][3] * vector.w
        );
    }
    FVector TransformPosition(const FVector& vector) const
    {
        float x = M[0][0] * vector.X + M[1][0] * vector.Y + M[2][0] * vector.Z + M[3][0];
        float y = M[0][1] * vector.X + M[1][1] * vector.Y + M[2][1] * vector.Z + M[3][1];
        float z = M[0][2] * vector.X + M[1][2] * vector.Y + M[2][2] * vector.Z + M[3][2];
        float w = M[0][3] * vector.X + M[1][3] * vector.Y + M[2][3] * vector.Z + M[3][3];
        return w != 0.0f ? FVector{ x / w, y / w, z / w } : FVector{ x, y, z };
    }
};