#include "ControlEditorPanel.h"

#include "Engine/World.h"
#include "Engine/Loader/OBJ/FLoaderOBJ.h"
#include "Engine/StaticMeshActor.h"
#include "LevelEditor/SLevelEditor.h"
#include "tinyfiledialogs/tinyfiledialogs.h"
#include "UnrealEd/EditorViewportClient.h"
#include "Classes/Actors/DirectionalLightActor.h"
#include "Classes/Actors/PointLightActor.h"
#include "Components/GameFramework/ProjectileMovementComponent.h"
#include "Serialization/FWindowsBinHelper.h"
#include "Actors/SpotLightActor.h"
#include <Actors/ExponentialHeightFog.h>
#include <UObject/UObjectIterator.h>

#include "LaunchEngineLoop.h"
#include "ShowFlags.h"
#include "Camera/CameraComponent.h"

#include "Components/PrimitiveComponents/Physics/UBoxShapeComponent.h"
#include "Components/PrimitiveComponents/Physics/USphereShapeComponent.h"
#include "Components/PrimitiveComponents/Physics/UCapsuleShapeComponent.h"
#include "Components/PrimitiveComponents/UParticleSubUVComp.h"
#include "Components/PrimitiveComponents/UTextComponent.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include "Components/USpringArmComponent.h"

#include "Contents/AGBullet.h"
#include "Contents/AGPlayer.h"
#include "ImGUI/imgui.h"
#include "Renderer/Renderer.h"
#include "UnrealEd/EditorPlayer.h"
#include "UObject/ObjectTypes.h"


void ControlEditorPanel::Initialize(SLevelEditor* levelEditor)
{
    activeLevelEditor = levelEditor;
}

void ControlEditorPanel::Render()
{
    /* Pre Setup */
    ImGuiIO& io = ImGui::GetIO();
    ImFont* IconFont = io.Fonts->Fonts[FEATHER_FONT];
    ImVec2 IconSize = ImVec2(32, 32);

    float PanelWidth = (Width) * 0.8f;
    float PanelHeight = 45.0f;

    float PanelPosX = 1.0f;
    float PanelPosY = 1.0f;

    ImVec2 MinSize(300, 50);
    ImVec2 MaxSize(FLT_MAX, 50);

    /* Min, Max Size */
    ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);

    /* Panel Position */
    ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY), ImGuiCond_Always);

    /* Panel Size */
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, PanelHeight), ImGuiCond_Always);

    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;

    /* Render Start */
    ImGui::Begin("Control Panel", nullptr, PanelFlags);

    CreateMenuButton(IconSize, IconFont);

    ImGui::SameLine();

    CreateFlagButton();

    ImGui::SameLine();

    CreateModifyButton(IconSize, IconFont);

    ImGui::SameLine();

    CreateShaderHotReloadButton(IconSize);

    ImGui::SameLine();
    
    ImVec2 PIEIconSize = ImVec2(IconSize.x + 8, IconSize.y);
    ImGui::PushFont(IconFont);
    CreatePIEButton(PIEIconSize);
    ImGui::PopFont();

    ImGui::SameLine();

    /* Get Window Content Region */
    float ContentWidth = ImGui::GetWindowContentRegionMax().x;

    /* Move Cursor X Position */
    ImGui::SetCursorPosX(ContentWidth - (IconSize.x * 3.0f + 16.0f));

    ImGui::PushFont(IconFont);
    CreateSRTButton(IconSize);
    ImGui::PopFont();

    ImGui::End();
}

void ControlEditorPanel::CreateMenuButton(ImVec2 ButtonSize, ImFont* IconFont)
{
    ImGui::PushFont(IconFont);
    if (ImGui::Button("\ue9ad", ButtonSize)) // Menu
    {
        bOpenMenu = !bOpenMenu;
    }
    ImGui::PopFont();

    if (bOpenMenu)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 55), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(135, 170), ImGuiCond_Always);

        ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        if (ImGui::MenuItem("New Scene"))
        {
            GEngine->GetWorld()->ReloadScene("Assets/Scenes/NewScene.scene");
        }

        if (ImGui::MenuItem("Load Scene"))
        {
            char const* lFilterPatterns[1] = { "*.scene" };
            const char* FileName = tinyfd_openFileDialog("Open Scene File", "Assets/Scenes", 1, lFilterPatterns, "Scene(.scene) file", 0);

            if (FileName == nullptr)
            {
                tinyfd_messageBox("Error", "파일을 불러올 수 없습니다.", "ok", "error", 1);
                ImGui::End();
                return;
            }
            GEngine->GetWorld()->ReloadScene(FileName);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save Scene"))
        {
            char const* lFilterPatterns[1] = { "*.scene" };
            const char* FileName = tinyfd_saveFileDialog("Save Scene File", "Assets/Scenes", 1, lFilterPatterns, "Scene(.scene) file");

            if (FileName == nullptr)
            {
                ImGui::End();
                return;
            }

            // TODO: Save Scene
            //int i = 1;
            //FArchive ar;
            UWorld World = *GEngine->GetWorld();
            World.SaveScene(FileName);
            // ar << World;
            //
            // FWindowsBinHelper::SaveToBin(FileName, ar);

            tinyfd_messageBox("알림", "저장되었습니다.", "ok", "info", 1);
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Import"))
        {
            if (ImGui::MenuItem("Wavefront (.obj)"))
            {
                char const* lFilterPatterns[1] = { "*.obj" };
                const char* FileName = tinyfd_openFileDialog("Open OBJ File", "", 1, lFilterPatterns, "Wavefront(.obj) file", 0);

                if (FileName != nullptr)
                {
                    std::cout << FileName << std::endl;

                    if (FManagerOBJ::CreateStaticMesh(FileName) == nullptr)
                    {
                        tinyfd_messageBox("Error", "파일을 불러올 수 없습니다.", "ok", "error", 1);
                    }
                }
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Quit"))
        {
            ImGui::OpenPopup("프로그램 종료");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("프로그램 종료", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("정말 프로그램을 종료하시겠습니까?");
            ImGui::Separator();

            float ContentWidth = ImGui::GetWindowContentRegionMax().x;

            /* Move Cursor X Position */
            ImGui::SetCursorPosX(ContentWidth - (160.f + 10.0f));

            if (ImGui::Button("OK", ImVec2(80, 0))) { PostQuitMessage(0); }

            ImGui::SameLine();

            ImGui::SetItemDefaultFocus();
            ImGui::PushID("CancelButtonWithQuitWindow");
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 1.0f, 1.0f));
            if (ImGui::Button("Cancel", ImVec2(80, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::PopStyleColor(3);
            ImGui::PopID();

            ImGui::EndPopup();
        }

        ImGui::End();
    }
}

void ControlEditorPanel::CreateModifyButton(ImVec2 ButtonSize, ImFont* IconFont)
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);

    if (EditorEngine == nullptr)
    {
        return;
    }
    
    ImGui::PushFont(IconFont);
    if (ImGui::Button("\ue9c4", ButtonSize)) // Slider
    {
        ImGui::OpenPopup("SliderControl");
    }
    ImGui::PopFont();

    if (ImGui::BeginPopup("SliderControl"))
    {
        ImGui::Text("Grid Scale");
        GridScale = EditorEngine->GetLevelEditor()->GetActiveViewportClient()->GridSize;
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##Grid Scale", &GridScale, 0.1f, 1.0f, 20.0f, "%.1f"))
        {
            EditorEngine->GetLevelEditor()->GetActiveViewportClient()->GridSize = GridScale;
        }
        ImGui::Separator();

        ImGui::Text("Camera FOV");
        FOV = &EditorEngine->GetLevelEditor()->GetActiveViewportClient()->ViewFOV;
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##Fov", FOV, 0.1f, 30.0f, 120.0f, "%.1f"))
        {
            //GEngineLoop.GetWorld()->GetCamera()->SetFOV(FOV);
        }
        ImGui::Spacing();

        ImGui::Text("Camera Speed");
        CameraSpeed = EditorEngine->GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar();
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##CamSpeed", &CameraSpeed, 0.1f, 0.198f, 192.0f, "%.1f"))
        {
            EditorEngine->GetLevelEditor()->GetActiveViewportClient()->SetCameraSpeed(CameraSpeed);
        }

        ImGui::Text("Blur Strength");
        float BlurStrength = EditorEngine->testBlurStrength;
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##Blur Strength", &BlurStrength, 0.02f, 0.0f, 5.0f, "%.1f"))
        {
            EditorEngine->testBlurStrength = BlurStrength;
        }
        ImGui::Separator();

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    ImGui::PushFont(IconFont);
    if (ImGui::Button("\ue9c8", ButtonSize))
    {
        ImGui::OpenPopup("ActorControl");
    }
    ImGui::PopFont();

    if (ImGui::BeginPopup("ActorControl"))
    {
        struct Actor {
            const char* category;
            const char* label;
            int obj;
        };

        // 카테고리 순서대로 정렬된 배열
        static const Actor actors[] = {
            { "Defaults", "Actor", OBJ_ACTOR},
            { "Defaults", "GamePlayer", OBJ_GAMEPLAYER},
            // 🔦 라이트
            { "Lights", "Spot Light",      OBJ_SPOTLIGHT },
            { "Lights", "Point Light",     OBJ_POINTLIGHT },
            { "Lights", "Directional Light", OBJ_DIRECTIONALLIGHT },

            // 🔷 셰이프
            { "Shapes", "Cube",            OBJ_CUBE },
            { "Shapes", "Sphere",          OBJ_SPHERE },
            { "Shapes", "Capsule",         OBJ_CAPSULE },
            { "Shapes", "Car (Dodge)",     OBJ_CAR },
            { "Shapes", "SkySphere",       OBJ_SKYSPHERE},
            { "Shapes", "Yeoul",           OBJ_YEOUL},

            // ✨ 효과
            { "Effects", "Particle",       OBJ_PARTICLE },
            { "Effects", "Text",           OBJ_TEXT },
            { "Effects", "Fog",            OBJ_FOG },
        };

        const char* currentCategory = nullptr;

        for (const auto& actor : actors)
        {
            // 카테고리 헤더 추가
            if (currentCategory != actor.category)
            {
                if (currentCategory != nullptr)
                {
                    ImGui::Separator(); // 카테고리 구분선
                }
                ImGui::PushFont(IconFont);
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1), "\ue9a8"); // 헤더
                ImGui::PopFont();
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1), "%s", actor.category); // 헤더
                currentCategory = actor.category;
            }

            // 액터 생성 버튼
            if (ImGui::Selectable(actor.label))
            {
                UWorld* World = GEngine->GetWorld();
                AActor* SpawnedActor = nullptr;

                switch (static_cast<OBJECTS>(actor.obj))
                {
                case OBJ_ACTOR:
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_ACTOR"));
                    SpawnedActor->AddComponent<USceneComponent>(EComponentOrigin::Editor);
                    break;
                case OBJ_GAMEPLAYER:
                {
                    SpawnedActor = World->SpawnActor<AGPlayer>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_GAMEPLAYER"));
                    FManagerOBJ::CreateStaticMesh("Assets/Primitives/Cube.obj");
                    UStaticMeshComponent* MeshComp = SpawnedActor->AddComponent<UStaticMeshComponent>(EComponentOrigin::Editor);
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Cube.obj"));
                    USpringArmComponent* SpringComp = SpawnedActor->AddComponent<USpringArmComponent>(EComponentOrigin::Editor);
                    UCameraComponent* Camera = SpawnedActor->AddComponent<UCameraComponent>(EComponentOrigin::Editor);
                    //SpawnedActor->AddComponent<USphereShapeComponent>(EComponentOrigin::Editor)->SetAttachParent(SpringComp);
                    SpringComp->SetTargetComponent(Camera);

                    SpawnedActor->AddComponent<UBoxShapeComponent>(EComponentOrigin::Editor);
                    break;
                }
                    //  셰이프
                case OBJ_CUBE:
                {
                    AStaticMeshActor* TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("Cube"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/Primitives/Cube.obj");
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Cube.obj"));
                    TempActor->AddComponent<UBoxShapeComponent>(EComponentOrigin::Editor);

                    SpawnedActor = TempActor;
                    break;
                }
                case OBJ_SPHERE:
                {
                    AStaticMeshActor* TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("Sphere"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/Primitives/Sphere.obj");
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Sphere.obj"));
                    TempActor->AddComponent<USphereShapeComponent>(EComponentOrigin::Editor);

                    SpawnedActor = TempActor;
                    break;
                }
                case OBJ_CAPSULE:
                {
                    AStaticMeshActor* TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("Capsule"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/Primitives/Capsule.obj");
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Capsule.obj"));
                    TempActor->AddComponent<UCapsuleShapeComponent>(EComponentOrigin::Editor);

                    SpawnedActor = TempActor;
                    break;
                }
                case OBJ_SKYSPHERE:
                {
                    //AStaticMeshActor* TempActor = World->SpawnActor<AStaticMeshActor>();
                    //TempActor->SetActorLabel(TEXT("OBJ_SKYSPHERE"));
                    //UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    //FManagerOBJ::CreateStaticMesh("Assets/SkySphere.obj");
                    //MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"SkySphere.obj"));
                    //TempActor->SetActorRotation(FVector(-90.0f, 0.0f, 0.0f));
                    //TempActor->SetActorScale(FVector(100.0f, 100.0f, 100.0f));
                    //SpawnedActor = TempActor;

                    AGBullet* TempActor = World->SpawnActor<AGBullet>();
                    TempActor->SetActorLabel(TEXT("Bullet"));

                    SpawnedActor = TempActor;

                    break; // 누락된 break 추가
                }

                case OBJ_POINTLIGHT:
                {
                    SpawnedActor = World->SpawnActor<APointLightActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_POINTLIGHT"));
                    break;
                }
                case OBJ_SPOTLIGHT:
                {
                    SpawnedActor = World->SpawnActor<ASpotLightActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_SpotLight"));
                    break;
                }
                case OBJ_DIRECTIONALLIGHT:
                {
                    SpawnedActor = World->SpawnActor<ADirectionalLightActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_DIRECTIONALLIGHT"));
                    break;
                }

                // ✨ 효과
                case OBJ_PARTICLE:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_PARTICLE"));
                    UParticleSubUVComp* Particle = SpawnedActor->AddComponent<UParticleSubUVComp>(EComponentOrigin::Editor);
                    Particle->SetTexture(L"Assets/Texture/T_Explosion_SubUV.png");
                    Particle->SetRowColumnCount(6, 6);
                    Particle->SetRelativeScale(FVector(10.0f, 10.0f, 10.0f));
                    Particle->Activate();
                    break;
                }
                case OBJ_TEXT:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_TEXT"));
                    UTextComponent* Text = SpawnedActor->AddComponent<UTextComponent>(EComponentOrigin::Editor);
                    Text->SetTexture(L"Assets/Texture/font.png");
                    Text->SetRowColumnCount(106, 106);
                    Text->SetText(L"안녕하세요 Jungle 1");
                    break;
                }
                case OBJ_FOG:
                {
                    for (const auto& actor : TObjectRange<AExponentialHeightFogActor>())
                    {
                        if (actor)
                        {
                            actor->Destroy();
                            TSet<AActor*> Actors = GEngine->GetWorld()->GetSelectedActors();
                            if(Actors.Contains(actor))
                                GEngine->GetWorld()->ClearSelectedActors();
                        }
                    }
                    SpawnedActor = World->SpawnActor<AExponentialHeightFogActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_FOG"));
                    break;
                }
                case OBJ_CAR:
                {
                    AStaticMeshActor* TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("OBJ_DODGE"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/Dodge/Dodge.obj");
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Dodge.obj"));
                    SpawnedActor = TempActor;
                    break;
                }
                case OBJ_YEOUL:
                {
                    AStaticMeshActor* TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("OBJ_YEOUL"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/Yeoul/yeoul.obj");
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"yeoul.obj"));
                    SpawnedActor = TempActor;
                    break;
                }
                }

                if (SpawnedActor)
                {
                    World->SetSelectedActor(SpawnedActor);
                }
            }
        }
        ImGui::EndPopup();
    }
}

void ControlEditorPanel::CreateFlagButton() const
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);

    if (EditorEngine == nullptr)
    {
        return;
    }
    auto ActiveViewport = EditorEngine->GetLevelEditor()->GetActiveViewportClient();

    const char* ViewTypeNames[] = { "Perspective", "Top", "Bottom", "Left", "Right", "Front", "Back" };
    ELevelViewportType ActiveViewType = ActiveViewport->GetViewportType();
    FString TextViewType = ViewTypeNames[ActiveViewType];

    if (ImGui::Button(GetData(TextViewType), ImVec2(120, 32)))
    {
        // toggleViewState = !toggleViewState;
        ImGui::OpenPopup("ViewControl");
    }

    if (ImGui::BeginPopup("ViewControl"))
    {
        for (int i = 0; i < IM_ARRAYSIZE(ViewTypeNames); i++)
        {
            bool bIsSelected = ((int)ActiveViewport->GetViewportType() == i);
            if (ImGui::Selectable(ViewTypeNames[i], bIsSelected))
            {
                ActiveViewport->SetViewportType((ELevelViewportType)i);
            }

            if (bIsSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    const char* ViewModeNames[] = { "Goroud_Lit", "Lambert_Lit", "Phong_Lit", "Unlit", "Wireframe", "Depth", "Normal"};
    FString SelectLightControl = ViewModeNames[static_cast<uint32>(ActiveViewport->GetViewMode())];
    ImVec2 LightTextSize = ImGui::CalcTextSize(GetData(SelectLightControl));

    if (ImGui::Button(GetData(SelectLightControl), ImVec2(30 + LightTextSize.x, 32)))
    {
        ImGui::OpenPopup("LightControl");
    }

    if (ImGui::BeginPopup("LightControl"))
    {
        for (int i = 0; i < IM_ARRAYSIZE(ViewModeNames); i++)
        {
            const bool bIsSelected = (static_cast<uint32>(ActiveViewport->GetViewMode()) == i);
            if (ImGui::Selectable(ViewModeNames[i], bIsSelected))
            {
                ActiveViewport->SetViewMode(static_cast<EViewModeIndex>(i));
                //UEditorEngine::renderer.SetViewMode(ActiveViewport->GetViewMode());
            }

            if (bIsSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Show", ImVec2(60, 32)))
    {
        ImGui::OpenPopup("ShowControl");
    }

    const char* items[] = { "AABB", "Primitive", "BillBoard", "UUID", "Fog" };
    uint64 ActiveViewportFlags = ActiveViewport->GetShowFlag();

    if (ImGui::BeginPopup("ShowControl"))
    {
        bool selected[IM_ARRAYSIZE(items)] =
        {
            (ActiveViewportFlags & static_cast<uint64>(EEngineShowFlags::SF_AABB)) != 0,
            (ActiveViewportFlags & static_cast<uint64>(EEngineShowFlags::SF_Primitives)) != 0,
            (ActiveViewportFlags & static_cast<uint64>(EEngineShowFlags::SF_BillboardText)) != 0,
            (ActiveViewportFlags & static_cast<uint64>(EEngineShowFlags::SF_UUIDText)) != 0,
            (ActiveViewportFlags & static_cast<uint64>(EEngineShowFlags::SF_Fog)) != 0,
        };  // 각 항목의 체크 상태 저장

        for (int i = 0; i < IM_ARRAYSIZE(items); i++)
        {
            ImGui::Checkbox(items[i], &selected[i]);
        }
        ActiveViewport->SetShowFlag(ConvertSelectionToFlags(selected));
        ImGui::EndPopup();
    }
}

void ControlEditorPanel::CreateShaderHotReloadButton(const ImVec2 ButtonSize) const
{
    ID3D11ShaderResourceView* IconTextureSRV = GEngineLoop.ResourceManager.GetTexture(L"Assets/Texture/HotReload.png")->TextureSRV;
    const ImTextureID textureID = reinterpret_cast<ImTextureID>(IconTextureSRV); // 실제 사용되는 텍스처 SRV
    if (ImGui::ImageButton("btn1", textureID, ButtonSize))
    {
        GEngineLoop.Renderer.GetResourceManager()->HotReloadShaders();
    }
}

void ControlEditorPanel::CreatePIEButton(ImVec2 ButtonSize) const
{
    float TotalWidth = ButtonSize.x * 3.0f + 16.0f;
    float ContentWidth = ImGui::GetWindowContentRegionMax().x;

    // 중앙 정렬을 위한 커서 위치 설정
    float CursorPosX = (ContentWidth - TotalWidth) * 0.5f;
    ImGui::SetCursorPosX(CursorPosX);

    if (activeLevelEditor->GetEditorStateManager().GetEditorState() == EEditorState::Editing)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
        if (ImGui::Button("\ue9a8", ButtonSize)) // Play
        {
            activeLevelEditor->GetEditorStateManager().SetState(EEditorState::PreparingPlay);
        }
        ImGui::PopStyleColor();
    }
    else if (activeLevelEditor->GetEditorStateManager().GetEditorState() == EEditorState::Paused)
    {
        if (ImGui::Button("\ue9a8", ButtonSize)) // Play
        {
            activeLevelEditor->GetEditorStateManager().SetState(EEditorState::Playing);
        }
    }
    else
    {
        if (ImGui::Button("\ue99c", ButtonSize)) // Pause
        {
            // TODO: PIE 일시정지
            activeLevelEditor->GetEditorStateManager().SetState(EEditorState::Paused);
        }
    }
    ImGui::SameLine();

    if (activeLevelEditor->GetEditorStateManager().GetEditorState() == EEditorState::Editing)
    {
        if (ImGui::Button("\ue9e4", ButtonSize)) // Stop
        {
            activeLevelEditor->GetEditorStateManager().SetState(EEditorState::Stopped);
        }
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        if (ImGui::Button("\ue9e4", ButtonSize)) // Stop
        {
            activeLevelEditor->GetEditorStateManager().SetState(EEditorState::Stopped);
        }
        ImGui::PopStyleColor();
    }
}

// code is so dirty / Please refactor
void ControlEditorPanel::CreateSRTButton(ImVec2 ButtonSize) const
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);

    if (EditorEngine == nullptr)
    {
        return;
    }
    
    UEditorPlayer* Player = EditorEngine->GetEditorPlayer();

    ImVec4 ActiveColor = ImVec4(0.00f, 0.00f, 0.85f, 1.0f);

    ControlMode ControlMode = Player->GetControlMode();

    if (ControlMode == CM_TRANSLATION)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9bc", ButtonSize)) // Move
    {
        Player->SetMode(CM_TRANSLATION);
    }
    if (ControlMode == CM_TRANSLATION)
    {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    if (ControlMode == CM_ROTATION)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9d3", ButtonSize)) // Rotate
    {
        Player->SetMode(CM_ROTATION);
    }
    if (ControlMode == CM_ROTATION)
    {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    if (ControlMode == CM_SCALE)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9ab", ButtonSize)) // Scale
    {
        Player->SetMode(CM_SCALE);
    }
    if (ControlMode == CM_SCALE)
    {
        ImGui::PopStyleColor();
    }
}

uint64 ControlEditorPanel::ConvertSelectionToFlags(const bool selected[]) const
{
    uint64 flags = static_cast<uint64>(EEngineShowFlags::None);

    if (selected[0])
        flags |= static_cast<uint64>(EEngineShowFlags::SF_AABB);
    if (selected[1])
        flags |= static_cast<uint64>(EEngineShowFlags::SF_Primitives);
    if (selected[2])
        flags |= static_cast<uint64>(EEngineShowFlags::SF_BillboardText);
    if (selected[3])
        flags |= static_cast<uint64>(EEngineShowFlags::SF_UUIDText);
    if (selected[4])
        flags |= static_cast<uint64>(EEngineShowFlags::SF_Fog);
    return flags;
}


void ControlEditorPanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    Width = clientRect.right - clientRect.left;
    Height = clientRect.bottom - clientRect.top;
}
