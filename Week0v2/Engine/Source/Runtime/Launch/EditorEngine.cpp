#include "EditorEngine.h"

#include "LaunchEngineLoop.h"
#include "WindowsCursor.h"
#include "Engine/World.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/UnrealEd.h"
#include "LevelEditor/SLevelEditor.h"
#include "UObject/UObjectIterator.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Contents/UI/ContentsUI.h"
#include "Coroutine/LuaCoroutine.h"
#include "GameFramework/Actor.h"
#include "Physics/FCollisionManager.h"
#include "Script/LuaManager.h"
#include "UnrealEd/EditorPlayer.h"
#include "UObject/Casts.h"
#include "Engine/AssetManager.h"
#include "UnrealEd/SkeletalPreviewUI.h"

class ULevel;


FCollisionManager UEditorEngine::CollisionManager;
FCoroutineManager UEditorEngine::CoroutineManager;

void UEditorEngine::Init()
{
    Super::Init();
    LevelEditor = new SLevelEditor();
    UnrealEditor = new UnrealEd();
    ContentsUI = new FContentsUI();
    SkeletalPreviewUI = new FSkeletalPreviewUI();

    UWorld* EditorWorld = CreateWorld(EWorldType::Editor, LEVELTICK_ViewportsOnly);
    
    /* must be initialized before window. */
    LevelEditor->Initialize(EditorWorld, GEngineLoop.GetDefaultWindow());

    UnrealEditor->Initialize(LevelEditor, GEngineLoop.GraphicDevice.GetDefaultWindowData().ScreenWidth, GEngineLoop.GraphicDevice.GetDefaultWindowData().ScreenHeight);
    SkeletalPreviewUI->Initialize(LevelEditor, GEngineLoop.GraphicDevice.GetDefaultWindowData().ScreenWidth, GEngineLoop.GraphicDevice.GetDefaultWindowData().ScreenHeight);
    ContentsUI->Initialize();
    CollisionManager.Initialize();  
    FLuaManager::Get().Initialize();    

    EditorPlayer = FObjectFactory::ConstructObject<UEditorPlayer>(this);
    EditorPlayer->Initialize();
    
    RegisterWaitHelpers(FLuaManager::Get().GetLuaState());

    if (AssetManager == nullptr)
    {
        AssetManager = FObjectFactory::ConstructObject<UAssetManager>(this);
        assert(AssetManager);
        AssetManager->InitAssetManager();
    }
}

void UEditorEngine::Tick(float DeltaTime)
{
    for (const auto& [World, WorldContext] : WorldContexts)
    {
        WorldContext->GetWorld()->Tick(WorldContext->LevelType, DeltaTime);
    }
    
    CollisionManager.UpdateCollision(DeltaTime);
    
    Input();
    
    LevelEditor->Tick(DeltaTime);

    CoroutineManager.Tick(DeltaTime);
    CoroutineManager.CleanupCoroutines();
}

void UEditorEngine::Release()
{
    LevelEditor->Release();
    
    for (const auto& [World, WorldContext] : WorldContexts)
    {
        World->Release();
    }
    WorldContexts.Empty();
    
    
    CollisionManager.Release();

    FLuaManager::Get().Shutdown();
    
    delete LevelEditor;
    delete UnrealEditor;
    delete SkeletalPreviewUI;
    delete ContentsUI;
}

void UEditorEngine::Input()
{
    if (GetAsyncKeyState('L') & 0x8000 or GetAsyncKeyState(VK_ESCAPE) & 0x8000)
    {
        LevelEditor->GetEditorStateManager().SetState(EEditorState::Stopped);
    }

    if (GetAsyncKeyState('P') & 0x8000 and GetAsyncKeyState(VK_MENU) & 0x8000)
    {
        if (LevelEditor->GetEditorStateManager().GetEditorState() == EEditorState::Editing)
        {
            LevelEditor->GetEditorStateManager().SetState(EEditorState::PreparingPlay);
        }
        // else if (LevelEditor->GetEditorStateManager().GetEditorState() == EEditorState::Paused)
        // {
        //     LevelEditor->GetEditorStateManager().SetState(EEditorState::Playing);
        // }
        // else
        // {
        //     LevelEditor->GetEditorStateManager().SetState(EEditorState::Paused);
        // }
    }

    if (GetAsyncKeyState('U') & 0x8000)
    {
        if (!bUButtonDown)
        {
            bUButtonDown= true;
            ForceEditorUIOnOff();
        }
    }
    else
    {
        if (bUButtonDown)
        {
            bUButtonDown = false;
        }
    }
}

void UEditorEngine::PreparePIE()
{
    UWorld* PIEWorld = Cast<UWorld>(EditorWorldContext->GetWorld()->Duplicate(this));
    PIEWorld->WorldType = EWorldType::PIE;
    PIEWorld->InitWorld();
    std::shared_ptr<FWorldContext> PIEWorldContext = CreateNewWorldContext(PIEWorld, EWorldType::PIE, LEVELTICK_All);
}

void UEditorEngine::StartPIE()
{
    // 1. BeingPlay() 호출
    PIEWorldContext->GetWorld()->BeginPlay();
    UE_LOG(LogLevel::Error, "Start PIE");
}

void UEditorEngine::PausedPIE()
{
    if (PIEWorldContext->LevelType == LEVELTICK_All)
    {
        PIEWorldContext->LevelType = LEVELTICK_PauseTick;
    }
    else if (PIEWorldContext->LevelType == LEVELTICK_PauseTick)
    {
        PIEWorldContext->LevelType = LEVELTICK_All;
    }
    UE_LOG(LogLevel::Error, "Pause PIE");
}

void UEditorEngine::ResumingPIE()
{

}

void UEditorEngine::StopPIE()
{
    if (PIEWorldContext == nullptr)
    {
        MessageBox(nullptr, L"PIE WORLD is not exist", nullptr, MB_OK | MB_ICONERROR);
        return;
    }

    WorldContexts.Remove(PIEWorldContext->GetWorld());
    
    PIEWorldContext->GetWorld()->Release();
    
    
    PIEWorldContext = nullptr;
}

void UEditorEngine::UpdateGizmos(UWorld* World)
{
    for (UGizmoBaseComponent* GizmoComp : TObjectRange<UGizmoBaseComponent>())
    {
        if (World != GizmoComp->GetWorld())
        {
            continue;
        }
        
        if (!World->GetSelectedActors().IsEmpty())
        {
            AActor* PickedActor = *World->GetSelectedActors().begin();
            if (PickedActor == nullptr || PickedActor->GetRootComponent() == nullptr)
            {
                break;
            }
            std::shared_ptr<FEditorViewportClient> activeViewport = GetLevelEditor()->GetActiveViewportClient();
            if (activeViewport->IsPerspective())
            {
                float scalar = abs(
                    (activeViewport->ViewTransformPerspective.GetLocation() - PickedActor->GetRootComponent()->GetRelativeLocation()).Magnitude()
                );
                scalar *= 0.1f;
                GizmoComp->SetRelativeScale(FVector(scalar, scalar, scalar));
            }
            else
            {
                float scalar = activeViewport->OrthoSize * 0.1f;
                GizmoComp->SetRelativeScale(FVector(scalar, scalar, scalar));
            }
        }
    }
}

UWorld* UEditorEngine::CreateWorld(EWorldType::Type WorldType, ELevelTick LevelTick)
{
    UWorld* World = FObjectFactory::ConstructObject<UWorld>(this);
    World->WorldType = WorldType;
    World->InitWorld();
    std::shared_ptr<FWorldContext> EditorContext = CreateNewWorldContext(World, WorldType, LevelTick);

    return World;
}

void UEditorEngine::RemoveWorld(UWorld* World)
{
    if (World == PreviewWorld)
    {
        PreviewWorld = nullptr;
    }
    World->Release();    
    WorldContexts.Remove(World);
}

UWorld* UEditorEngine::CreatePreviewWindow()
{
    if (PreviewWorld != nullptr)
    {
        return PreviewWorld;
    }
    
    WCHAR EnginePreviewWindowClass[] = L"PreviewWindowClass";
    WCHAR EnginePreviewTitle[] = L"Preview";

    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(GEngineLoop.GetDefaultWindow(), GWLP_HINSTANCE));
    HWND AppWnd = GEngineLoop.CreateEngineWindow(hInstance, EnginePreviewWindowClass, EnginePreviewTitle);
        
    PreviewWorld = CreateWorld(EWorldType::EditorPreview, LEVELTICK_ViewportsOnly);
        
    std::shared_ptr<FEditorViewportClient> EditorViewportClient = GetLevelEditor()->AddViewportClient<FEditorViewportClient>(AppWnd, PreviewWorld);
    EditorViewportClient->SetViewMode(VMI_Unlit);

    return PreviewWorld;
}

std::shared_ptr<FWorldContext> UEditorEngine::CreateNewWorldContext(UWorld* InWorld, EWorldType::Type InWorldType, ELevelTick LevelType)
{
    std::shared_ptr<FWorldContext> NewWorldContext = std::make_shared<FWorldContext>();
    NewWorldContext->WorldType = InWorldType;
    WorldContexts.Add(InWorld, NewWorldContext);
    

    NewWorldContext->LevelType = LevelType;
    NewWorldContext->SetWorld(InWorld);
    

    return NewWorldContext;
}
