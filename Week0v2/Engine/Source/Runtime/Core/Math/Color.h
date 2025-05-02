#pragma once
#include "MathUtility.h"
#include "Vector.h"
#include "Vector4.h"
#include "Container/String.h"
#include "Serialization/Archive.h"

/**
 * 0~255 사이의 값을 가지는 색상 구조체
 * BGRA 순서로 저장됨
 * 감마가 적용된 색상
 * sRGB 색상 공간을 사용
 * GPU 연산 시에는 이 값이 더 빠름.
 */
struct FColor
{
    union
    {
        struct
        {
            uint8 B;
            uint8 G;
            uint8 R;
            uint8 A;
        };
        uint32 Bits;
    };

    uint32& DWColor() { return Bits; }
    const uint32& DWColor() const { return Bits; }

    FColor() : B(0), G(0), R(0), A(255) {}
    FColor(const uint8 InR, const uint8 InG, const uint8 InB, const uint8 InA = 255) : B(InB), G(InG), R(InR), A(InA) {}
    FColor(const uint32 InColor)
    {
        DWColor() = InColor;
    }

    void Serialize(FArchive& Ar) const
    {
        Ar << R << G << B << A;
    }

    void Deserialize(FArchive& Ar)
    {
        Ar >> R >> G >> B >> A;
    }

    // Operators
    FORCEINLINE bool operator==(const FColor& Other) const
    {
        return DWColor() == Other.DWColor();
    }

    FORCEINLINE bool operator!=(const FColor& Other) const
    {
        return DWColor() != Other.DWColor();
    }

    FORCEINLINE void operator+=(const FColor& Other)
    {
        R = static_cast<uint8>(FMath::Min(static_cast<int32>(R) + static_cast<int32>(Other.R), 255));
        G = static_cast<uint8>(FMath::Min(static_cast<int32>(G) + static_cast<int32>(Other.G), 255));
        B = static_cast<uint8>(FMath::Min(static_cast<int32>(B) + static_cast<int32>(Other.B), 255));
        A = static_cast<uint8>(FMath::Min(static_cast<int32>(A) + static_cast<int32>(Other.A), 255));
    }


    static const FColor White;
    static const FColor Black;
    static const FColor Transparent;
    static const FColor Red;
    static const FColor Green;
    static const FColor Blue;
    static const FColor Yellow;
    static const FColor Cyan;
    static const FColor Magenta;
    static const FColor Orange;
    static const FColor Purple;
    static const FColor Turquoise;
    static const FColor Silver;
    static const FColor Emerald;
    static const FColor Gray;
};

// 0~1 사이의 값을 가지는 색상 구조체
struct FLinearColor
{
    union
    {
        struct
        {
            float R, G, B, A;
        };

        float RGBA[4];
    };

    FLinearColor() : R(0), G(0), B(0), A(0) {}
    //FLinearColor(float InR, float InG, float InB, float InA = 1.0f) : R(InR), G(InG), B(InB), A(InA) {}
    constexpr FLinearColor(const float InR, const float InG, const float InB, const float InA = 1.0f) : R(InR), G(InG), B(InB), A(InA) {}

    // FVector4 생성자와의 혼동을 피하기 위해 explicit으로 유지할 것.
    explicit FLinearColor(const FVector& InVector) : R(InVector.X), G(InVector.Y), B(InVector.Z), A(1.0f) {}

    // FVector 생성자와의 혼동을 피하기 위해 explicit으로 유지할 것.
    explicit FLinearColor(const FVector4& InVector) : R(InVector.x), G(InVector.y), B(InVector.z), A(InVector.w) {}

    constexpr FLinearColor(const FColor& InColor)
        : R(InColor.R / 255.0f), G(InColor.G / 255.0f), B(InColor.B / 255.0f), A(InColor.A / 255.0f) {
    }
    
    FLinearColor operator+(const FLinearColor& ColorB) const
    {
        return FLinearColor(R + ColorB.R, G + ColorB.G, B + ColorB.B, A + ColorB.A);
    }

    FLinearColor operator-(const FLinearColor& ColorB) const
    {
        return FLinearColor(R - ColorB.R, G - ColorB.G, B - ColorB.B, A - ColorB.A);
    }

    FLinearColor operator*(const FLinearColor& ColorB) const
    {
        return FLinearColor(R * ColorB.R, G * ColorB.G, B * ColorB.B, A * ColorB.A);
    }

    FLinearColor operator*(float Scalar) const
    {
        return FLinearColor(R * Scalar, G * Scalar, B * Scalar, A * Scalar);
    }

    FLinearColor operator/(float Scalar) const
    {
        const float InvScalar = 1.0f / Scalar;
        return FLinearColor(R * InvScalar, G * InvScalar, B * InvScalar, A * InvScalar);
    }

    bool operator==(const FLinearColor& ColorB) const
    {
        return R == ColorB.R && G == ColorB.G && B == ColorB.B && A == ColorB.A;
    }

    bool operator!=(const FLinearColor& Other) const
    {
        return R != Other.R || G != Other.G || B != Other.B || A != Other.A;
    }

    FLinearColor Clamp(const float InMin = 0.0f, const float InMax = 1.0f) const
    {
        return FLinearColor(
            FMath::Clamp(R, InMin, InMax),
            FMath::Clamp(G, InMin, InMax),
            FMath::Clamp(B, InMin, InMax),
            FMath::Clamp(A, InMin, InMax)
        );
    }

    static FLinearColor Lerp(const FLinearColor& A, const FLinearColor& B, float Alpha)
    {
        return FLinearColor(
            A.R + Alpha * (B.R - A.R),
            A.G + Alpha * (B.G - A.G),
            A.B + Alpha * (B.B - A.B),
            A.A + Alpha * (B.A - A.A)
        );
    }

    void Serialize(FArchive& Ar) const
    {
        Ar << R << G << B << A;
    }

    void Deserialize(FArchive& Ar)
    {
        Ar >> R >> G >> B >> A;
    }

    FORCEINLINE float GetMax() const
    {
        return FMath::Max(R, FMath::Max(G, FMath::Max(B, A)));
    }

    FORCEINLINE float GetMin() const
    {
        return FMath::Min(R, FMath::Min(G, FMath::Min(B, A)));
    }

    FORCEINLINE FVector rgb() const
    {
        return FVector(R, G, B);
    }

    static const FLinearColor White;
    static const FLinearColor Black;
    static const FLinearColor Red;
    static const FLinearColor Green;
    static const FLinearColor Blue;

    FString ToString() const;
};
