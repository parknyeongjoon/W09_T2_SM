#include "World.h"

#include "PlayerCameraManager.h"
#include "Actors/Player.h"
#include "BaseGizmos/TransformGizmo.h"
#include "Camera/CameraComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Engine/FLoaderOBJ.h"
#include "UObject/UObjectIterator.h"
#include "Level.h"
#include "Actors/ADodge.h"
#include "Contents/AGEnemy.h"
#include "Contents/GameManager.h"
#include "Serialization/FWindowsBinHelper.h"

#include "Actors/PointLightActor.h"
#include "Components/LightComponents/PointLightComponent.h"

UWorld::UWorld(const UWorld& Other): UObject(Other)
                                   , defaultMapName(Other.defaultMapName)
                                   , Level(Other.Level)
                                   , WorldType(Other.WorldType)
                                    , EditorPlayer(nullptr)
                                    , LocalGizmo(nullptr)
{
}

void UWorld::InitWorld()
{
    // TODO: Load Scene
    Level = FObjectFactory::ConstructObject<ULevel>();
    PreLoadResources();
    LoadScene("Assets/Scenes/AutoSave.Scene");
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
    if (EditorPlayer == nullptr)
    {
        EditorPlayer = FObjectFactory::ConstructObject<AEditorPlayer>();
    }
    
    if (LocalGizmo == nullptr)
    {
        LocalGizmo = FObjectFactory::ConstructObject<UTransformGizmo>();
    }

    
    PlayerCameraManager = SpawnActor<APlayerCameraManager>();
    
    APointLightActor* Light1 = SpawnActor<APointLightActor>();
    APointLightActor* Light2 = SpawnActor<APointLightActor>();
    APointLightActor* Light3 = SpawnActor<APointLightActor>();
    APointLightActor* Light4 = SpawnActor<APointLightActor>();

    Light1->SetActorLocation(FVector(-75, -75, 20));
    Light2->SetActorLocation(FVector(-75, 75, 20));
    Light3->SetActorLocation(FVector(75, -75, 20));
    Light4->SetActorLocation(FVector(75, 75, 20));

    Light1->GetComponentByClass<UPointLightComponent>()->SetColor(FVector4(0.8f, 0.05f, 0.05f, 1));
    Light1->GetComponentByClass<UPointLightComponent>()->SetRadius(50);
    Light1->GetComponentByClass<UPointLightComponent>()->SetIntensity(2);

    Light2->GetComponentByClass<UPointLightComponent>()->SetColor(FVector4(0.8f, 0.05f, 0.05f, 1));
    Light2->GetComponentByClass<UPointLightComponent>()->SetRadius(50);
    Light2->GetComponentByClass<UPointLightComponent>()->SetIntensity(2);

    Light3->GetComponentByClass<UPointLightComponent>()->SetColor(FVector4(0.8f, 0.05f, 0.05f, 1));
    Light3->GetComponentByClass<UPointLightComponent>()->SetRadius(50);
    Light3->GetComponentByClass<UPointLightComponent>()->SetIntensity(2);

    Light4->GetComponentByClass<UPointLightComponent>()->SetColor(FVector4(0.8f, 0.05f, 0.05f, 1));
    Light4->GetComponentByClass<UPointLightComponent>()->SetRadius(50);
    Light4->GetComponentByClass<UPointLightComponent>()->SetIntensity(2);
}

void UWorld::ReleaseBaseObject()
{
    LocalGizmo = nullptr;
    EditorPlayer = nullptr;
}

void UWorld::Tick(ELevelTick tickType, float deltaSeconds)
{
    if (tickType == LEVELTICK_ViewportsOnly)
    {
        if (EditorPlayer)
            EditorPlayer->Tick(deltaSeconds);
        if (LocalGizmo)
            LocalGizmo->Tick(deltaSeconds);
        
        FGameManager::Get().EditorTick(deltaSeconds);
    }
    // SpawnActor()에 의해 Actor가 생성된 경우, 여기서 BeginPlay 호출
    if (tickType == LEVELTICK_All)
    {
        FLuaManager::Get().BeginPlay();
        for (AActor* Actor : Level->PendingBeginPlayActors)
        {
            Actor->BeginPlay();
        }
        Level->PendingBeginPlayActors.Empty();

        // 매 틱마다 Actor->Tick(...) 호출
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
    SaveScene("Assets/Scenes/AutoSave.Scene");
	for (AActor* Actor : Level->GetActors())
	{
		Actor->EndPlay(EEndPlayReason::WorldTransition);
        TArray<UActorComponent*> Components = Actor->GetComponents();
	    for (UActorComponent* Component : Components)
	    {
	        GUObjectArray.MarkRemoveObject(Component);
	    }
	    GUObjectArray.MarkRemoveObject(Actor);
	}
    Level->GetActors().Empty();

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
        DestroyActor(actor);
    }
    Level->GetActors().Empty();
    Level->PendingBeginPlayActors.Empty();
    ReleaseBaseObject();
}

UObject* UWorld::Duplicate() const
{
    UWorld* CloneWorld = FObjectFactory::ConstructObjectFrom<UWorld>(this);
    CloneWorld->DuplicateSubObjects(this);
    CloneWorld->PostDuplicate();
    return CloneWorld;
}

void UWorld::DuplicateSubObjects(const UObject* SourceObj)
{
    UObject::DuplicateSubObjects(SourceObj);
    Level = Cast<ULevel>(Level->Duplicate());
    EditorPlayer = FObjectFactory::ConstructObject<AEditorPlayer>();
    LocalGizmo = FObjectFactory::ConstructObject<UTransformGizmo>();
}

void UWorld::PostDuplicate()
{
    UObject::PostDuplicate();
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

void UWorld::DuplicateSeletedActors()
{
    TSet<AActor*> newSelectedActors;
    for (AActor* Actor : SelectedActors)
    {
        AActor* DupedActor = Cast<AActor>(Actor->Duplicate());
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
void UWorld::DuplicateSeletedActorsOnLocation()
{
    TSet<AActor*> newSelectedActors;
    for (AActor* Actor : SelectedActors)
    {
        AActor* DupedActor = Cast<AActor>(Actor->Duplicate());
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
            AActor* Actor = SpawnActorByClass(ActorClass, true);
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

    for (auto Actor :GEngine->GetWorld()->GetActors())
    {
        if (APlayerCameraManager* CameraManager = Cast<APlayerCameraManager>(Actor))
        {
            PlayerCameraManager = CameraManager;
        }
    }

}

UWorld* UWorld::DuplicateWorldForPIE(UWorld* world)
{
    return new UWorld();
}

AActor* SpawnActorByName(const FString& ActorName, bool bCallBeginPlay)
{
    {
        UClass* ActorClass = UClassRegistry::Get().FindClassByName(ActorName);
        return GEngine->GetWorld()->SpawnActorByClass(ActorClass, bCallBeginPlay);
        
    }

}

/**********************************************************/
