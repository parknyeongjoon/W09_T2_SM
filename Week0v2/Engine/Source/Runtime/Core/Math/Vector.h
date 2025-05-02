#pragma once

#include <DirectXMath.h>

#include <cmath>

#include "MathUtility.h"

struct FMath;

#define SMALL_NUMBER         (1.e-8f)

#include "Serialization/Archive.h"
struct FVector2D
{
    float x, y;
    FVector2D(float _x = 0, float _y = 0) : x(_x), y(_y) {}

    FVector2D operator+(const FVector2D& rhs) const
    {
        return FVector2D(x + rhs.x, y + rhs.y);
    }
    FVector2D operator-(const FVector2D& rhs) const
    {
        return FVector2D(x - rhs.x, y - rhs.y);
    }
    FVector2D operator*(float rhs) const
    {
        return FVector2D(x * rhs, y * rhs);
    }
    FVector2D operator/(float rhs) const
    {
        return FVector2D(x / rhs, y / rhs);
    }
    FVector2D& operator+=(const FVector2D& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    void Serialize(FArchive& Ar) const
    {
        Ar << x << y;
    }
    void Deserialize(FArchive& Ar)
    {
        Ar >> x >> y;
    }
};

// 3D 벡터
struct FVector
{
    float X, Y, Z;
    FVector() : X(0), Y(0), Z(0) {}
    FVector(float _x, float _y, float _z) : X(_x), Y(_y), Z(_z) {}

    FVector operator+(const FVector& Other) const;
    FVector& operator+=(const FVector& Other);

    FVector operator-(const FVector& Other) const;
    FVector& operator-=(const FVector& Other);

    FVector operator*(const FVector& Other) const;
    FVector operator*(float Scalar) const;
    FVector& operator*=(float Scalar);

    FVector operator/(const FVector& Other) const;
    FVector operator/(float Scalar) const;
    FVector& operator/=(float Scalar);

    FVector operator-() const;

    // 인덱스 접근 연산자 추가
    float& operator[](int Index)
    {
        switch (Index)
        {
        case 0: return X;
        case 1: return Y;
        case 2: return Z;
        default: throw std::out_of_range("FVector index out of range");
        }
    }

    float operator[](int Index) const
    {
        switch (Index)
        {
        case 0: return X;
        case 1: return Y;
        case 2: return Z;
        default: throw std::out_of_range("FVector index out of range");
        }
    }

    // 벡터 내적
    float Dot(const FVector& other) const {
        return X * other.X + Y * other.Y + Z * other.Z;
    }

    // 벡터 크기
    float Magnitude() const {
        return std::sqrt(X * X + Y * Y + Z * Z);
    }

    float MagnitudeSquared() const
    {
        return X * X + Y * Y + Z * Z;
    }

    // 벡터 정규화
    FVector Normalize() const {
        float mag = Magnitude();
        return (mag > 0) ? FVector(X / mag, Y / mag, Z / mag) : FVector(0, 0, 0);
    }
    FVector Cross(const FVector& Other) const
    {
        return FVector{
            Y * Other.Z - Z * Other.Y,
            Z * Other.X - X * Other.Z,
            X * Other.Y - Y * Other.X
        };
    }
    FVector Min(const FVector& Other) const
    {
        FVector result = *this;
        if (X > Other.X)
            result.X = Other.X;
        if (Y > Other.Y)
            result.Y = Other.Y;
        if (Z > Other.Z)
            result.Z = Other.Z;

        return result;
    }

    FVector Max(const FVector& Other) const
    {
        FVector result = *this;
        if (X < Other.X)
            result.X = Other.X;
        if (Y < Other.Y)
            result.Y = Other.Y;
        if (Z < Other.Z)
            result.Z = Other.Z;

        return result;
    }

    float MaxValue() const
    {
        float result = X >= Y ? X : Y;
        result = result >= Z ? result : Z;
        return result;
    }

    bool operator==(const FVector& other) const {
        return (X == other.X && Y == other.Y && Z == other.Z);
    }

    float Distance(const FVector& other) const {
        // 두 벡터의 차 벡터의 크기를 계산
        return ((*this - other).Magnitude());
    }
    DirectX::XMFLOAT3 ToXMFLOAT3() const
    {
        return DirectX::XMFLOAT3(X, Y, Z);
    }

    FVector ClampMaxSize(float MaxSize) const
    {
        if (MaxSize < 1.e-4f)
        {
            return ZeroVector;
        }

        const float VSq = MagnitudeSquared();
        if (VSq > MaxSize * MaxSize)
        {
            const float Scale = MaxSize * FMath::InvSqrt(VSq);
            return { X * Scale, Y * Scale, Z * Scale };
        }
        else
        {
            return *this;
        }
    }
    void Serialize(FArchive& Ar) const
    {
        Ar << X << Y << Z;
    }

    void Deserialize(FArchive& Ar)
    {
        Ar >> X >> Y >> Z;
    }
    bool IsNearlyZero(float Tolerance = KINDA_SMALL_NUMBER) const
    {
        return FMath::Abs(X) <= Tolerance &&
            FMath::Abs(Y) <= Tolerance &&
            FMath::Abs(Z) <= Tolerance;
    }

    FVector GetClampedToMaxSize(float MaxSize) const;
    bool IsZero() const;
    FVector GetSafeNormal(float Tolerance = SMALL_NUMBER) const;
    
    static const FVector ZeroVector;
    static const FVector OneVector;
    static const FVector UpVector;
    static const FVector ForwardVector;
    static const FVector RightVector;
};
inline FVector FVector::operator+(const FVector& Other) const
{
    return { X + Other.X, Y + Other.Y, Z + Other.Z };
}

inline FVector& FVector::operator+=(const FVector& Other)
{
    X += Other.X; Y += Other.Y; Z += Other.Z;
    return *this;
}

inline FVector FVector::operator-(const FVector& Other) const
{
    return { X - Other.X, Y - Other.Y, Z - Other.Z };
}

inline FVector& FVector::operator-=(const FVector& Other)
{
    X -= Other.X; Y -= Other.Y; Z -= Other.Z;
    return *this;
}

inline FVector FVector::operator*(const FVector& Other) const
{
    return { X * Other.X, Y * Other.Y, Z * Other.Z };
}

inline FVector FVector::operator*(float Scalar) const
{
    return { X * Scalar, Y * Scalar, Z * Scalar };
}

inline FVector& FVector::operator*=(float Scalar)
{
    X *= Scalar; Y *= Scalar; Z *= Scalar;
    return *this;
}

inline FVector FVector::operator/(const FVector& Other) const
{
    return { X / Other.X, Y / Other.Y, Z / Other.Z };
}

inline FVector FVector::operator/(float Scalar) const
{
    return { X / Scalar, Y / Scalar, Z / Scalar };
}

inline FVector& FVector::operator/=(float Scalar)
{
    X /= Scalar; Y /= Scalar; Z /= Scalar;
    return *this;
}

inline FVector FVector::operator-() const
{
    return { -X, -Y, -Z };
}

inline FVector FVector::GetClampedToMaxSize(float MaxSize) const
{
    const float SquaredSum = X * X + Y * Y + Z * Z;
    const float MaxSizeSq = MaxSize * MaxSize;

    // 길이가 최대보다 크면
    if (SquaredSum > MaxSizeSq && MaxSizeSq > 0.f)
    {
        // 스케일 비율 = MaxSize / 현재 길이
        const float Scale = MaxSize / FMath::Sqrt(SquaredSum);
        return FVector(X * Scale, Y * Scale, Z * Scale);
    }

    // 아니면 원본 그대로
    return *this;
}

inline bool FVector::IsZero() const
{
    return X==0.f && Y==0.f && Z==0.f;
}

inline FVector FVector::GetSafeNormal(float Tolerance) const
{
    const float SquareSum = X*X + Y*Y + Z*Z;

    // Not sure if it's safe to add tolerance in there. Might introduce too many errors
    if (SquareSum == 1.f)
    {
        return *this;
    }
    else if (SquareSum < Tolerance)
    {
        return ZeroVector;
    }
    const float Scale = FMath::InvSqrt(SquareSum);
    return {X * Scale, Y * Scale, Z * Scale};
}