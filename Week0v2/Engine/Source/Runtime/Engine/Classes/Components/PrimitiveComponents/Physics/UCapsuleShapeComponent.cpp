#include "UCapsuleShapeComponent.h"
#include "Math/JungleMath.h"

UCapsuleShapeComponent::UCapsuleShapeComponent()
    : UShapeComponent()
    , CapsuleHalfHeight(0.5f) // Default capsule half height
    , CapsuleRadius(0.5f)     // Default capsule radius
{
}

UCapsuleShapeComponent::UCapsuleShapeComponent(const UCapsuleShapeComponent& Other)
    : UShapeComponent(Other)
    , CapsuleHalfHeight(Other.CapsuleHalfHeight)
    , CapsuleRadius(Other.CapsuleRadius)
{
}

UCapsuleShapeComponent::~UCapsuleShapeComponent()
{
}

void UCapsuleShapeComponent::InitializeComponent()
{
    Super::InitializeComponent();

    UpdateBroadAABB();
}

void UCapsuleShapeComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    if (PrevHalfHeight != CapsuleHalfHeight || PrevRadius != CapsuleRadius)
    {
        UpdateBroadAABB();

        PrevHalfHeight = CapsuleHalfHeight;
        PrevRadius = CapsuleRadius;
    }
}

const FShapeInfo* UCapsuleShapeComponent::GetShapeInfo() const
{
    FVector Center = GetWorldLocation();
    FMatrix WorldMatrix = GetWorldMatrix();

    // GetUpVector() 문제 있음. (회전 고치기 전이랑 같은 문제)
    FVector Up = FVector(WorldMatrix.M[2][0], WorldMatrix.M[2][1], WorldMatrix.M[2][2]);
    FMatrix RotationMatrix = GetWorldRotation().ToMatrix();
    float RadiusValue = GetRadius() * FMath::Max(GetWorldScale().X, GetWorldScale().Y);
    float HalfHeightValue = GetHalfHeight() * GetWorldScale().Z;

    ShapeInfo.Center = Center;
    ShapeInfo.WorldMatrix = WorldMatrix;
    ShapeInfo.Up = Up;
    ShapeInfo.RotationMatrix = RotationMatrix;
    ShapeInfo.Radius = RadiusValue;
    ShapeInfo.HalfHeight = HalfHeightValue;

    return &ShapeInfo;
}

void UCapsuleShapeComponent::UpdateBroadAABB()
{
    GetShapeInfo();

    FVector Center = ShapeInfo.Center;
    FMatrix WorldMatrix = ShapeInfo.WorldMatrix;
    FVector Up = ShapeInfo.Up;
    float R = ShapeInfo.Radius;
    float H = ShapeInfo.HalfHeight;

    FVector Top = Center + Up * H;
    FVector Bottom = Center - Up * H;

    FVector Min = Top - FVector(R, R, R);
    FVector Max = Top + FVector(R, R, R);

    FVector BottomMin = Bottom - FVector(R, R, R);
    FVector BottomMax = Bottom + FVector(R, R, R);

    Min.X = FMath::Min(Min.X, BottomMin.X);
    Min.Y = FMath::Min(Min.Y, BottomMin.Y);
    Min.Z = FMath::Min(Min.Z, BottomMin.Z);

    Max.X = FMath::Max(Max.X, BottomMax.X);
    Max.Y = FMath::Max(Max.Y, BottomMax.Y);
    Max.Z = FMath::Max(Max.Z, BottomMax.Z);

    BroadAABB.min = Min;
    BroadAABB.max = Max;
}

bool UCapsuleShapeComponent::TestOverlaps(const UShapeComponent* OtherShape) const
{
    if (!BroadPhaseCollisionCheck(OtherShape))
    {
        return false;
    }

    return NarrowPhaseCollisionCheck(OtherShape);
}

bool UCapsuleShapeComponent::NarrowPhaseCollisionCheck(const UShapeComponent* OtherShape) const
{
    if (!OtherShape)
    {
        return false;
    }

    if (OtherShape->IsA<UBoxShapeComponent>())
    {
        const UBoxShapeComponent* OtherBox = dynamic_cast<const UBoxShapeComponent*>(OtherShape);
        return CollisionCheckWithBox(OtherBox);
    }
    else if (OtherShape->IsA<USphereShapeComponent>())
    {
        const USphereShapeComponent* OtherSphere = dynamic_cast<const USphereShapeComponent*>(OtherShape);
        return CollisionCheckWithSphere(OtherSphere);
    }
    else if (OtherShape->IsA<UCapsuleShapeComponent>())
    {
        const UCapsuleShapeComponent* OtherCapsule = dynamic_cast<const UCapsuleShapeComponent*>(OtherShape);
        return CollisionCheckWithCapsule(OtherCapsule);
    }

    return false;
}

bool UCapsuleShapeComponent::CollisionCheckWithBox(const UBoxShapeComponent* OtherBox) const
{
    // 1. Capsule 세그먼트 엔드포인트 계산
    FVector CenterC = GetWorldLocation();
    float HalfH = GetHalfHeight();
    float RadiusC = GetRadius() * GetWorldScale().MaxValue();
    // 캡슐 로컬 Z축을 기준으로 세그먼트 방향 정의
    FMatrix RotC = GetRotationMatrix();
    FVector AxisC = FVector(RotC.M[0][2], RotC.M[1][2], RotC.M[2][2]);
    FVector P0 = CenterC + AxisC * HalfH;
    FVector P1 = CenterC - AxisC * HalfH;

    // 2. 박스 로컬 좌표로 변환
    FVector CenterB = OtherBox->GetWorldLocation();
    FMatrix RotB = OtherBox->GetRotationMatrix();
    FVector AxisBX(RotB.M[0][0], RotB.M[1][0], RotB.M[2][0]);
    FVector AxisBY(RotB.M[0][1], RotB.M[1][1], RotB.M[2][1]);
    FVector AxisBZ(RotB.M[0][2], RotB.M[1][2], RotB.M[2][2]);
    auto ToLocal = [&](const FVector& P) {
        FVector d = P - CenterB;
        return FVector(d.Dot(AxisBX), d.Dot(AxisBY), d.Dot(AxisBZ));
        };
    FVector L0 = ToLocal(P0);
    FVector L1 = ToLocal(P1);

    // 3. 확장된 AABB extents
    FVector ExtentB = OtherBox->GetBoxExtent() * GetWorldScale();
    FVector Exp = ExtentB + FVector(RadiusC, RadiusC, RadiusC);

    // 4. 세그먼트-AABB 교차 (슬랩 방법)
    FVector dLocal = L1 - L0;
    float tmin = 0.0f, tmax = 1.0f;
    for (int i = 0; i < 3; ++i)
    {
        float p0 = (i == 0 ? L0.X : (i == 1 ? L0.Y : L0.Z));
        float di = (i == 0 ? dLocal.X : (i == 1 ? dLocal.Y : dLocal.Z));
        float minB = (i == 0 ? -Exp.X : (i == 1 ? -Exp.Y : -Exp.Z));
        float maxB = (i == 0 ? Exp.X : (i == 1 ? Exp.Y : Exp.Z));
        if (FMath::Abs(di) < KINDA_SMALL_NUMBER)
        {
            if (p0 < minB || p0 > maxB)
                return false;
        }
        else
        {
            float ood = 1.0f / di;
            float t1 = (minB - p0) * ood;
            float t2 = (maxB - p0) * ood;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            tmin = FMath::Max(tmin, t1);
            tmax = FMath::Min(tmax, t2);
            if (tmin > tmax)
                return false;
        }
    }
    return true;
}

bool UCapsuleShapeComponent::CollisionCheckWithSphere(const USphereShapeComponent* OtherSphere) const
{
    FVector SphereCenter = OtherSphere->GetWorldLocation();
    float SphereRadius = OtherSphere->GetRadius() * OtherSphere->GetWorldScale().MaxValue();

    FVector CapsuleCenter = GetWorldLocation();
    FMatrix CapsuleRotation = GetRotationMatrix();
    FVector CapsuleUp = FVector(CapsuleRotation.M[0][1], CapsuleRotation.M[1][1], CapsuleRotation.M[2][1]); // Y축 기준

    float CapsuleHalfHeight = GetHalfHeight() * GetWorldScale().Y;
    float CapsuleRadius = GetRadius() * GetWorldScale().MaxValue();

    // 캡슐의 세그먼트 끝점 계산
    FVector Top = CapsuleCenter + CapsuleUp * CapsuleHalfHeight;
    FVector Bottom = CapsuleCenter - CapsuleUp * CapsuleHalfHeight;

    // 구 중심에서 세그먼트에 가장 가까운 점 구하기
    FVector ClosestPt = JungleMath::ClosestPointOnSegment(Bottom, Top, SphereCenter);

    // 거리 비교
    FVector d = SphereCenter - ClosestPt;
    float DistSquared = d.Dot(d);
    float CombinedRadius = SphereRadius + CapsuleRadius;

    return DistSquared <= CombinedRadius * CombinedRadius;
}

bool UCapsuleShapeComponent::CollisionCheckWithCapsule(const UCapsuleShapeComponent* OtherCapsule) const
{
    FVector CenterA = GetWorldLocation();
    FMatrix RotA = GetRotationMatrix();
    FVector UpA(RotA.M[0][1], RotA.M[1][1], RotA.M[2][1]);

    float HalfHeightA = GetHalfHeight() * GetWorldScale().Y;
    float RadiusA = GetRadius() * GetWorldScale().MaxValue();

    FVector TopA = CenterA + UpA * HalfHeightA;
    FVector BottomA = CenterA - UpA * HalfHeightA;

    FVector CenterB = OtherCapsule->GetWorldLocation();
    FMatrix RotB = OtherCapsule->GetRotationMatrix();
    FVector UpB(RotB.M[0][1], RotB.M[1][1], RotB.M[2][1]);

    float HalfHeightB = OtherCapsule->GetHalfHeight() * OtherCapsule->GetWorldScale().Y;
    float RadiusB = OtherCapsule->GetRadius() * OtherCapsule->GetWorldScale().MaxValue();

    FVector TopB = CenterB + UpB * HalfHeightB;
    FVector BottomB = CenterB - UpB * HalfHeightB;

    // 두 세그먼트 사이 최근접 점 찾기
    FVector ClosestA, ClosestB;  
    JungleMath::FindClosestPointsBetweenSegments(BottomA, TopA, BottomB, TopB, ClosestA, ClosestB);

    FVector d = ClosestA - ClosestB;
    float DistSquared = d.Dot(d);
    float CombinedRadius = RadiusA + RadiusB;

    return DistSquared <= CombinedRadius * CombinedRadius;
}
