#include "CubeComp.h"
#include "Math/JungleMath.h"
#include "Engine/World.h"

#include "UnrealEd/EditorViewportClient.h"

#include "Engine/Loader/OBJ/FLoaderOBJ.h"

UCubeComp::UCubeComp()
{
    AABB.max = { 1,1,1 };
    AABB.min = { -1,-1,-1 };

}

UCubeComp::UCubeComp(UCubeComp& Other)
    : Super(Other)
{
}

UCubeComp::~UCubeComp()
{
}

void UCubeComp::InitializeComponent()
{
    Super::InitializeComponent();

    FManagerOBJ::CreateStaticMesh("Assets/helloBlender.obj");
    SetStaticMesh(FManagerOBJ::GetStaticMesh(L"helloBlender.obj"));
}

void UCubeComp::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

}