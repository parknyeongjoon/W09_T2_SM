#include "World.h"

#include "LaunchEngineLoop.h"
#include "Renderer/Renderer.h"
#include "PlayerCameraManager.h"
#include "BaseGizmos/TransformGizmo.h"
#include "Camera/CameraComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Engine/FLoaderOBJ.h"
#include "UObject/UObjectIterator.h"
#include "Level.h"
#include "TestFBXLoader.h"
#include "Actors/ADodge.h"
#include "Contents/GameManager.h"
#include "Serialization/FWindowsBinHelper.h"

#include "Actors/PointLightActor.h"
#include "Components/LightComponents/PointLightComponent.h"
#include "Components/Mesh/StaticMesh.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/SkySphereComponent.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include "Components/PrimitiveComponents/Physics/UBoxShapeComponent.h"
#include "Script/LuaManager.h"
#include "UObject/UObjectArray.h"
#include "UnrealEd/PrimitiveBatch.h"

UWorld::UWorld(const UWorld& Other): UObject(Other)
                                   , defaultMapName(Other.defaultMapName)
                                   , Level(Other.Level)
                                   , WorldType(Other.WorldType)
                                   , LocalGizmo(nullptr)
{
}

void UWorld::InitWorld()
{
    // TODO: Load Scene
    Level = FObjectFactory::ConstructObject<ULevel>(this);
    PreLoadResources();
    if (WorldType == EWorldType::Editor)
    {
        LoadScene("Assets/Scenes/NewScene.Scene");
    }
    else if (WorldType == EWorldType::EditorPreview)
    {
        CreateBaseObject();
    }
}

void UWorld::LoadLevel(const FString& LevelName)
{
    // !TODO : 레벨 로드
    // 이름으로 레벨 로드한다
    // 실패 하면 현재 레벨 유지
}

void UWorld::PreLoadResources()
{
    FManagerOBJ::CreateStaticMesh(TEXT("Assets/CastleObj.obj"));
}

void UWorld::CreateBaseObject()
{
    if (LocalGizmo == nullptr)
    {
        LocalGizmo = FObjectFactory::ConstructObject<UTransformGizmo>(this);
    }
    
    PlayerCameraManager = SpawnActor<APlayerCameraManager>();
}


void UWorld::ReleaseBaseObject()
{
    LocalGizmo = nullptr;
}

void UWorld::Tick(ELevelTick tickType, float deltaSeconds)
{
    if (tickType == LEVELTICK_ViewportsOnly)
    {
        if (LocalGizmo)
        {
            LocalGizmo->Tick(deltaSeconds);
        }
        
        FGameManager::Get().EditorTick(deltaSeconds);
    }
    // SpawnActor()에 의해 Actor가 생성된 경우, 여기서 BeginPlay 호출
    if (tickType == LEVELTICK_All)
    {
        FLuaManager::Get().BeginPlay();
        TSet<AActor*> PendingActors = Level->PendingBeginPlayActors;
        for (AActor* Actor : PendingActors)
        {
            Actor->BeginPlay();
            Level->PendingBeginPlayActors.Remove(Actor);
        }

        TArray CopyActors = Level->GetActors();
        for (AActor* Actor : CopyActors)
        {
            Actor->Tick(deltaSeconds);
        }

        FGameManager::Get().Tick(deltaSeconds);
    }
}

void UWorld::Release()
{
    if (WorldType == EWorldType::Editor)
    {
        SaveScene("Assets/Scenes/AutoSave.Scene");
    }
    TArray<AActor*> Actors = Level->GetActors();
    for (AActor* Actor : Actors)
    {
        Actor->Destroy();
    }
    if (LocalGizmo)
    {
        LocalGizmo->Destroy();
    }

    GUObjectArray.MarkRemoveObject(Level);
    // TODO Level -> Release로 바꾸기
    // Level->Release();
    GUObjectArray.MarkRemoveObject(this);

	pickingGizmo = nullptr;
	ReleaseBaseObject();

    GUObjectArray.ProcessPendingDestroyObjects();
}

void UWorld::ClearScene()
{
    // 1. PickedActor제거
    SelectedActors.Empty();
    // 2. 모든 Actor Destroy
    
    for (AActor* actor : TObjectRange<AActor>())
    {
        if (actor->GetWorld() == this)
        {
            DestroyActor(actor);
        }
    }
    Level->GetActors().Empty();
    Level->PendingBeginPlayActors.Empty();
    ReleaseBaseObject();
}

UObject* UWorld::Duplicate(UObject* InOuter)
{
    UWorld* CloneWorld = FObjectFactory::ConstructObjectFrom<UWorld>(this, InOuter);
    CloneWorld->DuplicateSubObjects(this, InOuter);
    CloneWorld->PostDuplicate();
    return CloneWorld;
}

void UWorld::DuplicateSubObjects(const UObject* SourceObj, UObject* InOuter)
{
    UObject::DuplicateSubObjects(SourceObj, InOuter);
    Level = Cast<ULevel>(Level->Duplicate(InOuter));
    LocalGizmo = FObjectFactory::ConstructObject<UTransformGizmo>(InOuter);
}

void UWorld::PostDuplicate()
{
    UObject::PostDuplicate();
}

UWorld* UWorld::GetWorld() const
{
    return const_cast<UWorld*>(this);
}

void UWorld::ReloadScene(const FString& FileName)
{
    ClearScene(); // 기존 오브젝트 제거
    CreateBaseObject();
    FArchive ar;
    FWindowsBinHelper::LoadFromBin(FileName, ar);

    ar >> *this;
}

void UWorld::LoadScene(const FString& FileName)
{
    CreateBaseObject();
    FArchive ar;
    FWindowsBinHelper::LoadFromBin(FileName, ar);

    ar >> *this;
}

void UWorld::DuplicateSelectedActors()
{
    TSet<AActor*> newSelectedActors;
    for (AActor* Actor : SelectedActors)
    {
        AActor* DupedActor = Cast<AActor>(Actor->Duplicate(this));
        FString TypeName = DupedActor->GetActorLabel().Left(DupedActor->GetActorLabel().Find("_", ESearchCase::IgnoreCase,ESearchDir::FromEnd));
        DupedActor->SetActorLabel(TypeName);
        FVector DupedLocation = DupedActor->GetActorLocation();
        DupedActor->SetActorLocation(FVector(DupedLocation.X+50, DupedLocation.Y+50, DupedLocation.Z));
        Level->GetActors().Add(DupedActor);
        Level->PendingBeginPlayActors.Add(DupedActor);
        newSelectedActors.Add(DupedActor);
    }
    SelectedActors = newSelectedActors;
}
void UWorld::DuplicateSelectedActorsOnLocation()
{
    TSet<AActor*> newSelectedActors;
    for (AActor* Actor : SelectedActors)
    {
        AActor* DupedActor = Cast<AActor>(Actor->Duplicate(this));
        FString TypeName = DupedActor->GetActorLabel().Left(DupedActor->GetActorLabel().Find("_", ESearchCase::IgnoreCase,ESearchDir::FromEnd));
        DupedActor->SetActorLabel(TypeName);
        Level->GetActors().Add(DupedActor);
        Level->PendingBeginPlayActors.Add(DupedActor);
        newSelectedActors.Add(DupedActor);
    }
    SelectedActors = newSelectedActors;
}

bool UWorld::DestroyActor(AActor* ThisActor)
{
    if (ThisActor->GetWorld() == nullptr)
    {
        return false;
    }

    if (ThisActor->IsActorBeingDestroyed())
    {
        return true;
    }

    // 액터의 Destroyed 호출
    ThisActor->Destroyed();

    if (ThisActor->GetOwner())
    {
        ThisActor->SetOwner(nullptr);
    }

    TArray<UActorComponent*> Components = ThisActor->GetComponents();
    for (UActorComponent* Component : Components)
    {
        Component->DestroyComponent();
    }

    // World에서 제거
    Level->GetActors().Remove(ThisActor);

    // 제거 대기열에 추가
    GUObjectArray.MarkRemoveObject(ThisActor);
    return true;
}


void UWorld::SetPickingGizmo(UObject* Object)
{
	pickingGizmo = Cast<USceneComponent>(Object);
}

void UWorld::Serialize(FArchive& ar) const
{
    int ActorCount = Level->GetActors().Num();
    ar << ActorCount;
    for (AActor* Actor : Level->GetActors())
    {
        FActorInfo ActorInfo = (Actor->GetActorInfo());
        ar << ActorInfo;
    }
}

void UWorld::Deserialize(FArchive& ar)
{
    int ActorCount;
    ar >> ActorCount;
    for (int i = 0; i < ActorCount; i++)
    {
        FActorInfo ActorInfo;
        ar >> ActorInfo;
        UClass* ActorClass = UClassRegistry::Get().FindClassByName(ActorInfo.Type);
        if (ActorClass)
        {
            AActor* Actor = SpawnActorByClass(ActorClass, this, true);
            if (Actor)
            {
                Actor->LoadAndConstruct(ActorInfo.ComponentInfos);
                Level->GetActors().Add(Actor);
            }
        }
    }
    Level->PostLoad();
    
}

/*************************임시******************************/
bool UWorld::IsPIEWorld() const
{
    return false;
}

void UWorld::BeginPlay()
{
    FGameManager::Get().BeginPlay();

    for (auto Actor :GetActors())
    {
        if (APlayerCameraManager* CameraManager = Cast<APlayerCameraManager>(Actor))
        {
            PlayerCameraManager = CameraManager;
        }
    }
}

// AActor* SpawnActorByName(const FString& ActorName, UObject* InOuter, bool bCallBeginPlay)
// {
//     // GetWorld 문제 발생 여지 많음.
//     UClass* ActorClass = UClassRegistry::Get().FindClassByName(ActorName);
//     return GetWorld()->SpawnActorByClass(ActorClass, InOuter, bCallBeginPlay);
// }

/**********************************************************/
