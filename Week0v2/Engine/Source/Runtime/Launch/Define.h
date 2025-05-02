#pragma once
#include <cmath>
#include <algorithm>
#include "Core/Container/String.h"
#include "Core/Container/Array.h"
#include "UObject/NameTypes.h"

// 수학 관련
#include "Math/Vector.h"
#include "Math/Vector4.h"

#define UE_LOG Console::GetInstance().AddLog

#define _TCHAR_DEFINED
#include <d3d11.h>

#include "Math/Matrix.h"
#include "UserInterface/Console.h"
#include "Serialization/Archive.h"


struct FVertexSimple
{
    float x, y, z, w;    // Position
    float r, g, b, a; // Color
    float nx, ny, nz;
    float Tangentnx, Tangentny, Tangentnz;
    float u=0, v=0;
    uint32 MaterialIndex;
};

// Material Subset
struct FMaterialSubset
{
    uint32 IndexStart; // Index Buffer Start pos
    uint32 IndexCount; // Index Count
    uint32 MaterialIndex; // Material Index
    FString MaterialName; // Material Name
};

struct FStaticMaterial
{
    class UMaterial* Material;
    FName MaterialSlotName;
    //FMeshUVChannelInfo UVChannelData;
};

// OBJ File Raw Data
struct FObjInfo
{
    FWString ObjectName; // OBJ File Name
    FWString PathName; // OBJ File Paths
    FString DisplayName; // Display Name
    FString MatName; // OBJ MTL File Name
    
    // Group
    uint32 NumOfGroup = 0; // token 'g' or 'o'
    TArray<FString> GroupName;
    
    // Vertex, UV, Normal List
    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    
    // Faces
    TArray<int32> Faces;

    // Index
    TArray<uint32> VertexIndices;
    TArray<uint32> NormalIndices;
    TArray<uint32> TextureIndices;
    
    // Material
    TArray<FMaterialSubset> MaterialSubsets;
};

struct FObjMaterialInfo
{
    FString MTLName;  // newmtl : Material Name.

    bool bHasTexture = false;  // Has Texture?
    bool bTransparent = false; // Has alpha channel?

    // Diffuse (Kd) : 일반적으로 흰색, 완전 불투명한 색상
    FVector Diffuse = FVector(0.0f, 0.0f, 0.0f);
    
    // Specular (Ks) : 반사광 기본값, 흰색으로 표기하는 경우가 많음
    FVector Specular = FVector(0.0f, 0.0f, 0.0f);
    
    // Ambient (Ka) : 주변광 기본값, 너무 강하지 않은 낮은 값으로
    FVector Ambient = FVector(0.1f, 0.1f, 0.1f);
    
    // Emissive (Ke) : 자체 발광 없음
    FVector Emissive = FVector(0.0f, 0.0f, 0.0f);
    
    // SpecularScalar (Ns) : 스페큘러 파워 (보통 1.0 이상, 필요에 따라 조정)
    float SpecularScalar = 1.0f;
    
    // DensityScalar (Ni) : 광학적 밀도(굴절률 등), 기본적으로 1.0
    float DensityScalar = 1.0f;
    
    // TransparencyScalar : 투명도, 1.0이면 불투명, 0.0이면 완전 투명
    float TransparencyScalar = 1.0f;

    uint32 IlluminanceModel; // illum: illumination Model between 0 and 10. (UINT)

    /* Texture */
    FString DiffuseTextureName;  // map_Kd : Diffuse texture
    FWString DiffuseTexturePath;
    
    FString AmbientTextureName;  // map_Ka : Ambient texture
    FWString AmbientTexturePath;
    
    FString SpecularTextureName; // map_Ks : Specular texture
    FWString SpecularTexturePath;
    
    FString BumpTextureName;     // map_Bump : Bump texture
    FWString BumpTexturePath;
    
    FString AlphaTextureName;    // map_d : Alpha texture
    FWString AlphaTexturePath;

    FString NormalTextureName;  // map_Ns : Normal Texture
    FWString NormalTexturePath;

    float NormalScale = 1.0f;
};

// Cooked Data
namespace OBJ
{
    struct FStaticMeshRenderData
    {
        FWString ObjectName;
        FWString PathName;
        FString DisplayName;
        
        TArray<FVertexSimple> Vertices;
        TArray<UINT> Indices;

        FString VBName;
        FString IBName;
        // ID3D11Buffer* VertexBuffer;
        // ID3D11Buffer* IndexBuffer;
        
        TArray<FObjMaterialInfo> Materials;
        TArray<FMaterialSubset> MaterialSubsets;

        FVector BoundingBoxMin;
        FVector BoundingBoxMax;
    };
}

enum class EShaderStage
{
    VS,		// Vertex Shader
    HS,		// Hull Shader
    DS,		// Domain Shader
    GS,		// Geometry Shader
    PS,		// Pixel Shader
    CS,		// Compute Shader
    All,
    End,
};

// ShaderType과 Constant 이름을 결합한 키 구조체
struct FShaderConstantKey
{
    EShaderStage ShaderType;  // 예: Vertex, Pixel 등
    FName ConstantName;    // 상수 버퍼 내 상수 이름

    // 동등 비교 연산자: 두 키가 동일하면 true
    bool operator==(const FShaderConstantKey& Other) const
    {
        return ShaderType == Other.ShaderType && ConstantName == Other.ConstantName;
    }
};

// std::hash 특수화를 통해 FShaderConstantKey를 해시 기반 컨테이너에서 사용할 수 있게 함
namespace std
{
    template<>
    struct hash<FShaderConstantKey>
    {
        std::size_t operator()(const FShaderConstantKey& Key) const noexcept
        {
            // EShaderType은 enum class이므로 int로 캐스팅하여 해시를 계산
            std::size_t h1 = std::hash<uint32>()(static_cast<int>(Key.ShaderType));
            std::size_t h2 = std::hash<FName>()(Key.ConstantName);
            // 간단한 해시 결합: XOR과 쉬프트 사용 (더 복잡한 해시 결합도 가능)
            return h1 ^ (h2 << 1);
        }
    };
}

enum class ESamplerType
{
    Point,
    Linear,
    Anisotropic,
    PostProcess,
    ComparisonSampler, // Compare
    End,
};

enum class ERenderingMode
{
    Opaque,
    CutOut,
    Transparent,
    PostProcess,
    End,
};

enum class ETextureType
{
    Albedo,
    Normal,
    Specular,
    Smoothness,
    Metallic,
    Sprite,
    End,
};

enum class ERasterizerState
{
    SolidBack,
    SolidFront,
    SolidNone,
    WireFrame,
    End,
};

enum class EBlendState
{
    AlphaBlend,
    OneOne,
    End,
};

enum class EDepthStencilState
{
    DepthNone,
    LessEqual,
    End,
};

enum EShadowFilterMode
{
    PCF,
    VSM
};

struct FVertexTexture
{
	float x, y, z;    // Position
	float u, v; // Texture
};

struct FSimpleVertex
{
	float dummy; // 내용은 사용되지 않음
    float padding[11];
};

struct FOBB
{
    FVector corners[8];
};

struct FRect
{
    FRect() : leftTopX(0), leftTopY(0), width(0), height(0) {}
    FRect(float x, float y, float w, float h) : leftTopX(x), leftTopY(y), width(w), height(h) {}
    float leftTopX, leftTopY, width, height;
};

struct FPoint
{
    FPoint() : x(0), y(0) {}
    FPoint(float _x, float _y) : x(_x), y(_y) {}
    FPoint(long _x, long _y) : x(_x), y(_y) {}
    FPoint(int _x, int _y) : x(_x), y(_y) {}

    float x, y;
};
struct FBoundingBox
{
    FBoundingBox(){}
    FBoundingBox(FVector _min, FVector _max) : min(_min), max(_max) {}
	FVector min; // Minimum extents
	float pad;
	FVector max; // Maximum extents
	float pad1;
    bool IntersectRay(const FVector& rayOrigin, const FVector& rayDir, float& outDistance) const
    {
        float tmin = -FLT_MAX;
        float tmax = FLT_MAX;
        const float epsilon = 1e-6f;

        // X축 처리
        if (fabs(rayDir.X) < epsilon)
        {
            // 레이가 X축 방향으로 거의 평행한 경우,
            // 원점의 x가 박스 [min.x, max.x] 범위 밖이면 교차 없음
            if (rayOrigin.X < min.X || rayOrigin.X > max.X)
                return false;
        }
        else
        {
            float t1 = (min.X - rayOrigin.X) / rayDir.X;
            float t2 = (max.X - rayOrigin.X) / rayDir.X;
            if (t1 > t2)  std::swap(t1, t2);

            // tmin은 "현재까지의 교차 구간 중 가장 큰 min"
            tmin = (t1 > tmin) ? t1 : tmin;
            // tmax는 "현재까지의 교차 구간 중 가장 작은 max"
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        // Y축 처리
        if (fabs(rayDir.Y) < epsilon)
        {
            if (rayOrigin.Y < min.Y || rayOrigin.Y > max.Y)
                return false;
        }
        else
        {
            float t1 = (min.Y - rayOrigin.Y) / rayDir.Y;
            float t2 = (max.Y - rayOrigin.Y) / rayDir.Y;
            if (t1 > t2)  std::swap(t1, t2);

            tmin = (t1 > tmin) ? t1 : tmin;
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        // Z축 처리
        if (fabs(rayDir.Z) < epsilon)
        {
            if (rayOrigin.Z < min.Z || rayOrigin.Z > max.Z)
                return false;
        }
        else
        {
            float t1 = (min.Z - rayOrigin.Z) / rayDir.Z;
            float t2 = (max.Z - rayOrigin.Z) / rayDir.Z;
            if (t1 > t2)  std::swap(t1, t2);

            tmin = (t1 > tmin) ? t1 : tmin;
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        // 여기까지 왔으면 교차 구간 [tmin, tmax]가 유효하다.
        // tmax < 0 이면, 레이가 박스 뒤쪽에서 교차하므로 화면상 보기엔 교차 안 한다고 볼 수 있음
        if (tmax < 0.0f)
            return false;

        // outDistance = tmin이 0보다 크면 그게 레이가 처음으로 박스를 만나는 지점
        // 만약 tmin < 0 이면, 레이의 시작점이 박스 내부에 있다는 의미이므로, 거리를 0으로 처리해도 됨.
        outDistance = (tmin >= 0.0f) ? tmin : 0.0f;

        return true;
    }
    bool IntersectAABB(FBoundingBox Other) const
    {
        return (min.X <= Other.max.X && max.X >= Other.min.X) &&
            (min.Y <= Other.max.Y && max.Y >= Other.min.Y) &&
            (min.Z <= Other.max.Z && max.Z >= Other.min.Z);
    }
    void Serialize(FArchive& Ar) const
    {
        Ar << min << max;
    }
    void Deserialize(FArchive& Ar)
    {
        Ar >> min >> max;
    }
};
struct FCone
{
    FVector ConeApex; // 원뿔의 꼭짓점
    float ConeRadius; // 원뿔 밑면 반지름

    FVector ConeBaseCenter; // 원뿔 밑면 중심
    float ConeHeight; // 원뿔 높이 (Apex와 BaseCenter 간 차이)
    FVector4 Color;

    int ConeSegmentCount; // 원뿔 밑면 분할 수
    float pad[3];
};
struct FSphere
{
    FVector Center; 
    float Radius;
    FVector4 Color;
};
struct FLine
{
    FVector Start;
    float Length;
    FVector Direction;
    float pad;
    FVector4 Color;
};

struct FCapsule
{
    FVector Center;
    float Radius;
    FVector UpVector;
    float HalfHeight;
    FVector4 Color;
};

struct FPlane
{
    // 평면 방정식: Ax + By + Cz + D = 0
    float A, B, C, D;

    // 기본 생성자
    FPlane() : A(0), B(0), C(0), D(0) {}

    // 값들을 인자로 받아 생성하는 생성자
    FPlane(float InA, float InB, float InC, float InD)
        : A(InA), B(InB), C(InC), D(InD) {}

    // 평면을 정규화 하는 함수: 평면의 법선(A, B, C)의 길이를 1로 만듭니다.
    void Normalize()
    {
        float Magnitude = std::sqrt(A * A + B * B + C * C);
        if (Magnitude != 0.0f)
        {
            A /= Magnitude;
            B /= Magnitude;
            C /= Magnitude;
            D /= Magnitude;
        }
    }

    // 주어진 점(p)와 평면 사이의 signed distance를 계산합니다.
    // 양수 값이면 점이 평면의 법선 방향 쪽에 있고,
    // 음수 값이면 반대 방향에 있음을 나타냅니다.
    float PlaneDot(const FVector& p) const
    {
        return A * p.X + B * p.Y + C * p.Z + D;
    }
};
struct FFrustum
{
    // 프러스텀 평면이 6개 있다고 가정 (Left, Right, Bottom, Top, Near, Far)
    FPlane Planes[6];

    // 구의 (center, radius)를 기준으로 프러스텀과 교차하는지 검사하는 함수
    bool IntersectsSphere(const FVector& Center, float Radius) const
    {
        // 각 평면에 대해 구의 중심과의 거리가 -Radius보다 작으면 완전히 외부에 있다고 봅니다.
        for (int i = 0; i < 6; ++i)
        {
            if (Planes[i].PlaneDot(Center) < -Radius)
            {
                return false;
            }
        }
        return true;
    }
    bool IntersectsPoint(const FVector& Point) const
    {
        // 6개의 평면 모두에 대해 점이 평면의 '앞쪽'에 있어야 프러스텀 내부로 판단
        for (int i = 0; i < 6; ++i)
        {
            if (Planes[i].PlaneDot(Point) < 0)
            {
                return false;
            }
        }
        return true;
    }
    static FFrustum ExtractFrustum(const FMatrix& Mat) {
        FFrustum frustum;
    
        // Left Plane = M[3] + M[0]
        frustum.Planes[0] = FPlane(
            Mat.M[0][3] + Mat.M[0][0],
            Mat.M[1][3] + Mat.M[1][0],
            Mat.M[2][3] + Mat.M[2][0],
            Mat.M[3][3] + Mat.M[3][0]
        );
    
        // Right Plane = M[3] - M[0]
        frustum.Planes[1] = FPlane(
            Mat.M[0][3] - Mat.M[0][0],
            Mat.M[1][3] - Mat.M[1][0],
            Mat.M[2][3] - Mat.M[2][0],
            Mat.M[3][3] - Mat.M[3][0]
        );
    
        // Bottom Plane = M[3] + M[1]
        frustum.Planes[2] = FPlane(
            Mat.M[0][3] + Mat.M[0][1],
            Mat.M[1][3] + Mat.M[1][1],
            Mat.M[2][3] + Mat.M[2][1],
            Mat.M[3][3] + Mat.M[3][1]
        );
    
        // Top Plane = M[3] - M[1]
        frustum.Planes[3] = FPlane(
            Mat.M[0][3] - Mat.M[0][1],
            Mat.M[1][3] - Mat.M[1][1],
            Mat.M[2][3] - Mat.M[2][1],
            Mat.M[3][3] - Mat.M[3][1]
        );
    
        // Near Plane = M[3] + M[2]
        frustum.Planes[4] = FPlane(
            Mat.M[0][3] + Mat.M[0][2],
            Mat.M[1][3] + Mat.M[1][2],
            Mat.M[2][3] + Mat.M[2][2],
            Mat.M[3][3] + Mat.M[3][2]
        );
    
        // Far Plane = M[3] - M[2]
        frustum.Planes[5] = FPlane(
            Mat.M[0][3] - Mat.M[0][2],
            Mat.M[1][3] - Mat.M[1][2],
            Mat.M[2][3] - Mat.M[2][2],
            Mat.M[3][3] - Mat.M[3][2]
        );
    
        // 각 평면 정규화
        for (auto& plane : frustum.Planes) {
            plane.Normalize();
        }
    
        return frustum;
    }
};

// !NOTE : 최대 광원 수 제한
#define MAX_DIRECTIONAL_LIGHTS 4
#define MAX_POINT_LIGHTS 16
constexpr UINT CASCADE_COUNT = 4;

enum class ELightType
{
    None,
    DirectionalLight,
    PointLight,
    SpotLight,
    Max,
};

struct FDirectionalLight
{
    FVector Direction;
    float Intensity;

    FVector4 Color;

    float CascadeSplit[CASCADE_COUNT];

    FMatrix View[CASCADE_COUNT];
    FMatrix Projection[CASCADE_COUNT];

    uint32 CastShadow;
    FVector Pad;
};

struct FPointLight
{
    FVector Position;
    float Radius;

    FVector4 Color;

    float Intensity;
    float AttenuationFalloff;
    uint32 CastShadow;
    float Padd;

    FMatrix PointLightView[6];
    FMatrix PointLightProj;
};

struct FComputeConstants{
    FMatrix InverseView;
    FMatrix InverseProj;
    float screenWidth;
    float screenHeight;
    int tileCountX;
    int tileCountY;
};
struct FSpotLight
{
    FVector Position;
    float Intensity;

    FVector4 Color;

    FVector Direction;
    float InnerAngle;

    float OuterAngle;
    uint32 CastShadow;
    FVector2D pad;

    FMatrix View;
    FMatrix Proj;
    FVector4 AtlasUVTransform;
};

struct FIntRect
{
    int32 X;
    int32 Y;
    int32 Width;
    int32 Height;
    FIntRect() : X(0), Y(0), Width(0), Height(0) {}
    FIntRect(int32 InX, int32 InY, int32 InWidth, int32 InHeight)
        : X(InX), Y(InY), Width(InWidth), Height(InHeight) {
    }
};
