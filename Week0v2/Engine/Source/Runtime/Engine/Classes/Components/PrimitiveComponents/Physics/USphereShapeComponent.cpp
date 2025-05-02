#include "USphereShapeComponent.h"
#include "Math/JungleMath.h"
#include "UBoxShapeComponent.h"
#include "UCapsuleShapeComponent.h"

USphereShapeComponent::USphereShapeComponent()
    : UShapeComponent()
    , ShapeInfo()
    , Radius(0.5f) // Default radius
{
}

USphereShapeComponent::USphereShapeComponent(const USphereShapeComponent& Other)
    : UShapeComponent(Other)
    , ShapeInfo(Other.ShapeInfo)
    , PrevRadius(Other.PrevRadius)
    , Radius(Other.Radius)
{
}

USphereShapeComponent::~USphereShapeComponent()
{
}

void USphereShapeComponent::InitializeComponent()
{
    Super::InitializeComponent();

    UpdateBroadAABB();
}

void USphereShapeComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    if (PrevRadius != Radius)
    {
        UpdateBroadAABB();

        PrevRadius = Radius;
    }
}

UObject* USphereShapeComponent::Duplicate() const
{
    USphereShapeComponent* NewComp = FObjectFactory::ConstructObjectFrom<USphereShapeComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();

    return NewComp;
}

void USphereShapeComponent::DuplicateSubObjects(const UObject* Source)
{
    Super::DuplicateSubObjects(Source);
}

void USphereShapeComponent::PostDuplicate()
{
    Super::PostDuplicate();
}

const FShapeInfo* USphereShapeComponent::GetShapeInfo() const
{
    FVector Center = GetWorldLocation();
    float Scale = GetWorldScale().MaxValue();
    float ScaledRadius = GetRadius() * Scale;

    ShapeInfo.Center = Center;
    ShapeInfo.Radius = ScaledRadius;

    return &ShapeInfo;
}

void USphereShapeComponent::UpdateBroadAABB()
{
    GetShapeInfo();

    FVector Center = ShapeInfo.Center;
    float ScaledRadius = ShapeInfo.Radius;

    //FVector Center = GetComponentLocation();
    //float Scale = GetComponentScale().MaxValue();

    //float R = GetRadius();
    //float ScaledRadius = Scale * R;

    FVector LocalCorners[8] = {
        { ScaledRadius,  ScaledRadius,  ScaledRadius },
        { ScaledRadius,  ScaledRadius, -ScaledRadius },
        { ScaledRadius, -ScaledRadius,  ScaledRadius },
        { ScaledRadius, -ScaledRadius, -ScaledRadius },
        { -ScaledRadius,  ScaledRadius,  ScaledRadius },
        { -ScaledRadius,  ScaledRadius, -ScaledRadius },
        { -ScaledRadius, -ScaledRadius,  ScaledRadius },
        { -ScaledRadius, -ScaledRadius, -ScaledRadius }
    };

    FVector Min = LocalCorners[7] + Center;
    FVector Max = LocalCorners[0] + Center;

    BroadAABB.min = Min;
    BroadAABB.max = Max;
}

bool USphereShapeComponent::TestOverlaps(const UShapeComponent* OtherShape) const
{
    if (!BroadPhaseCollisionCheck(OtherShape))
    {
        return false;
    }

    return NarrowPhaseCollisionCheck(OtherShape);
}

bool USphereShapeComponent::NarrowPhaseCollisionCheck(const UShapeComponent* OtherShape) const
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

bool USphereShapeComponent::CollisionCheckWithBox(const UBoxShapeComponent* OtherBox) const
{
    // 1. Sphere 정보
    FVector CenterS = GetWorldLocation();
    float RadiusS = GetRadius() * GetWorldScale().MaxValue();

    // 2. Box OBB 정보
    FVector CenterB = OtherBox->GetWorldLocation();
    FVector ExtentB = OtherBox->GetBoxExtent() * GetWorldScale();
    FMatrix RotB = GetRotationMatrix();

    // 3. Sphere 중심을 Box 로컬 좌표로 변환
    FVector d = CenterS - CenterB;
    FVector AxisBX(RotB.M[0][0], RotB.M[1][0], RotB.M[2][0]);
    FVector AxisBY(RotB.M[0][1], RotB.M[1][1], RotB.M[2][1]);
    FVector AxisBZ(RotB.M[0][2], RotB.M[1][2], RotB.M[2][2]);

    float x = d.Dot(AxisBX);
    float y = d.Dot(AxisBY);
    float z = d.Dot(AxisBZ);

    // 4. Box 내부로의 투영점(Clamp)
    float cx = FMath::Clamp(x, -ExtentB.X, ExtentB.X);
    float cy = FMath::Clamp(y, -ExtentB.Y, ExtentB.Y);
    float cz = FMath::Clamp(z, -ExtentB.Z, ExtentB.Z);

    // 5. 최근접 점과 Sphere 중심 거리 계산
    float dx = x - cx;
    float dy = y - cy;
    float dz = z - cz;
    float Dist2 = dx * dx + dy * dy + dz * dz;

    // 6. 반지름 비교: 거리가 Sphere 반지름보다 작으면 충돌
    return Dist2 <= RadiusS * RadiusS;
}

bool USphereShapeComponent::CollisionCheckWithSphere(const USphereShapeComponent* OtherSphere) const
{
    FVector CenterA = GetWorldLocation();
    float RadiusA = GetRadius() * GetWorldScale().MaxValue();

    FVector CenterB = OtherSphere->GetWorldLocation();
    float RadiusB = OtherSphere->GetRadius() * OtherSphere->GetWorldScale().MaxValue();

    FVector d = CenterA - CenterB;
    float DistSquared = d.Dot(d);
    float RadiusSum = RadiusA + RadiusB;

    return DistSquared <= RadiusSum * RadiusSum;
}

bool USphereShapeComponent::CollisionCheckWithCapsule(const UCapsuleShapeComponent* OtherCapsule) const
{
    FVector SphereCenter = GetWorldLocation();
    float SphereRadius = GetRadius() * GetWorldScale().MaxValue();

    FVector CapsuleCenter = OtherCapsule->GetWorldLocation();
    FMatrix CapsuleRotation = OtherCapsule->GetRotationMatrix();
    FVector CapsuleUp = FVector(CapsuleRotation.M[0][1], CapsuleRotation.M[1][1], CapsuleRotation.M[2][1]); // Y축 기준

    float CapsuleHalfHeight = OtherCapsule->GetHalfHeight() * OtherCapsule->GetWorldScale().Y;
    float CapsuleRadius = OtherCapsule->GetRadius() * OtherCapsule->GetWorldScale().MaxValue();

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