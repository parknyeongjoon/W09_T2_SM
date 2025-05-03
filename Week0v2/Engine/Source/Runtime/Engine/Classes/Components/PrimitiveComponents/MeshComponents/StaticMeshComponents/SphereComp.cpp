#include "SphereComp.h"

#include "Engine/Loader/OBJ/FLoaderOBJ.h"
#include "UnrealEd/EditorViewportClient.h"


USphereComp::USphereComp()
{
    AABB.max = {1, 1, 1};
    AABB.min = {-1, -1, -1};
}

USphereComp::USphereComp(const USphereComp& Other)
    : Super(Other)
{
}

USphereComp::~USphereComp()
{
}

void USphereComp::InitializeComponent()
{
    Super::InitializeComponent();
    FManagerOBJ::CreateStaticMesh("Assets/Sphere.obj");
    SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Sphere.obj"));
}

void USphereComp::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}