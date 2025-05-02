#pragma once
#include "Math/Vector.h"
#include "Serialization/Archive.h"
// 4D Vector
struct FVector4 {
    float x, y, z, w;
    FVector4(float _x = 0, float _y = 0, float _z = 0, float _a = 0) : x(_x), y(_y), z(_z), w(_a) {}
    FVector4(FVector v, float w) : x(v.X), y(v.Y), z(v.Z), w(w) {}

    FVector4 operator-(const FVector4& other) const {
        return FVector4(x - other.x, y - other.y, z - other.z, w - other.w);
    }
    FVector4 operator+(const FVector4& other) const {
        return FVector4(x + other.x, y + other.y, z + other.z, w + other.w);
    }
    FVector4 operator/(float scalar) const
    {
        return FVector4{ x / scalar, y / scalar, z / scalar, w / scalar };
    }
    FVector xyz() const { return FVector(x, y, z); }
    void Serialize(FArchive& Ar) const
    {
        Ar << x << y << z << w;
    }
    void Deserialize(FArchive& Ar)
    {
        Ar >> x >> y >> z >> w;
    }
};
