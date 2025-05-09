#include "EditorViewportClient.h"

#include "EditorPlayer.h"
#include "ImGUI/imgui.h"

#include "Math/JungleMath.h"
#include "LaunchEngineLoop.h"
#include "Viewport.h"
#include "Engine/FLoaderOBJ.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "Components/LightComponents/DirectionalLightComponent.h"
#include "Components/LightComponents/SpotLightComponent.h"
#include "Engine/FEditorStateManager.h"
#include "LevelEditor/SLevelEditor.h"
#include "SlateCore/Layout/SlateRect.h"

FVector FEditorViewportClient::Pivot = FVector(0.0f, 0.0f, 0.0f);
float FEditorViewportClient::OrthoSize = 10.0f;

FEditorViewportClient::FEditorViewportClient() : Viewport(nullptr), ViewportType(LVT_Perspective), ShowFlag(31 | (1ULL << 10)), ViewMode(VMI_Lit_Phong)
{

}

FEditorViewportClient::~FEditorViewportClient()
{
    Release();
}

void FEditorViewportClient::Draw(FViewport* Viewport)
{
}

UWorld* FEditorViewportClient::GetWorld() const
{
    return World;
}

void FEditorViewportClient::Initialize(HWND InOwnerWindow, uint32 InViewportIndex, UWorld* World)
{
    SetOwner(InOwnerWindow);
    SetWorld(World);
    ViewportIndex = InViewportIndex;
    
    ViewTransformPerspective.SetLocation(FVector(8.0f, 8.0f, 8.f));
    ViewTransformPerspective.SetRotation(FVector(0.0f, 45.0f, -135.0f));
    Viewport = new FViewport();
    FWindowData& WindowData = GEngineLoop.GraphicDevice.SwapChains[InOwnerWindow];
    ResizeViewport(FRect(0, 0, WindowData.ScreenWidth, WindowData.ScreenHeight));
}

void FEditorViewportClient::Tick(float DeltaTime)
{
    if (GetWorld()->WorldType == EWorldType::Editor || GetWorld()->WorldType == EWorldType::EditorPreview)
    {
        UpdateEditorCameraMovement(DeltaTime);
    }
    
    UpdateViewMatrix();
    UpdateProjectionMatrix();
    UpdateCascadeShadowArea();

    // ----- Test Start
    // if (DebugCube.IsEmpty())
    // {
    //     for (int i=0;i<CASCADE_COUNT*8;i++)
    //     {
    //         AStaticMeshActor* TempActor = GetWorld()->SpawnActor<AStaticMeshActor>();
    //         TempActor->SetActorLabel(TEXT("OBJ_CUBE"));
    //         UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
    //         FManagerOBJ::CreateStaticMesh("Assets/Cube.obj");
    //         MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Cube.obj"));
    //         DebugCube.Add(TempActor);
    //     }
    // }
    //
    // for (int i=0;i<CASCADE_COUNT;i++)
    // {
    //     for (int j=0;j<8;j++)
    //     {
    //         DebugCube[i*8+j]->SetActorLocation(cascadeCorners[i][j]);
    //     }
    // }
    // ----- Test Finish
    
    // UEditorEngine::renderer.GetConstantBufferUpdater().UpdateCameraConstant(
    //     UEditorEngine::renderer.CameraConstantBuffer,
    //     this
    // );
}

void FEditorViewportClient::Release()
{
    delete Viewport;
}

void FEditorViewportClient::UpdateEditorCameraMovement(const float DeltaTime)
{
    if (CameraInputPressedKeys.Contains(EKeys::A))
    {
        CameraMoveRight(-100.f * DeltaTime);
    }

    if (CameraInputPressedKeys.Contains(EKeys::D))
    {
        CameraMoveRight(100.f * DeltaTime);
    }

    if (CameraInputPressedKeys.Contains(EKeys::W))
    {
        CameraMoveForward(100.f * DeltaTime);
    }

    if (CameraInputPressedKeys.Contains(EKeys::S))
    {
        CameraMoveForward(-100.f * DeltaTime);
    }

    if (CameraInputPressedKeys.Contains(EKeys::E))
    {
        CameraMoveUp(100.f * DeltaTime);
    }

    if (CameraInputPressedKeys.Contains(EKeys::Q))
    {
        CameraMoveUp(-100.f * DeltaTime);
    }
}

void FEditorViewportClient::InputKey(HWND AppWnd, const FKeyEvent& InKeyEvent)
{
    // TODO: 나중에 InKeyEvent.GetKey();로 가져오는걸로 수정하기
    // TODO: 나중에 PIEViewportClient에서 처리하는걸로 수정하기
    // if (GEngine->ActiveWorld->WorldType == EWorldType::PIE)
    // {
    //     // PIE 모드 → 게임 플레이 입력 처리
    //     UWorld* PlayWorld = GEngine->ActiveWorld;
    //     if (PlayWorld)
    //     {
    //         // 첫 번째 플레이어 컨트롤러에게 전달
    //         // if (APlayerController* PC = PlayWorld->GetFirstPlayerController())
    //         // {
    //         //     
    //         // }
    //     }
    // }
    // // 에디터 모드
    // else
    {
        // TODO: 나중에 InKeyEvent.GetKey();로 가져오는걸로 수정하기
        // 마우스 우클릭이 되었을때만 실행되는 함수
        if (GetKeyState(VK_RBUTTON) & 0x8000)
        {
            switch (InKeyEvent.GetCharacter())
            {
            case 'A':
            {
                if (InKeyEvent.GetInputEvent() == IE_Pressed)
                {
                    CameraInputPressedKeys.Add(EKeys::A);
                }
                else if (InKeyEvent.GetInputEvent() == IE_Released)
                {
                    CameraInputPressedKeys.Remove(EKeys::A);
                }
                break;
            }
            case 'D':
            {
                if (InKeyEvent.GetInputEvent() == IE_Pressed)
                {
                    CameraInputPressedKeys.Add(EKeys::D);
                }
                else if (InKeyEvent.GetInputEvent() == IE_Released)
                {
                    CameraInputPressedKeys.Remove(EKeys::D);
                }
                break;
            }
            case 'W':
            {
                if (InKeyEvent.GetInputEvent() == IE_Pressed)
                {
                    CameraInputPressedKeys.Add(EKeys::W);
                }
                else if (InKeyEvent.GetInputEvent() == IE_Released)
                {
                    CameraInputPressedKeys.Remove(EKeys::W);
                }
                break;
            }
            case 'S':
            {
                if (InKeyEvent.GetInputEvent() == IE_Pressed)
                {
                    CameraInputPressedKeys.Add(EKeys::S);
                }
                else if (InKeyEvent.GetInputEvent() == IE_Released)
                {
                    CameraInputPressedKeys.Remove(EKeys::S);
                }
                break;
            }
            case 'E':
            {
                if (InKeyEvent.GetInputEvent() == IE_Pressed)
                {
                    CameraInputPressedKeys.Add(EKeys::E);
                }
                else if (InKeyEvent.GetInputEvent() == IE_Released)
                {
                    CameraInputPressedKeys.Remove(EKeys::E);
                }
                break;
            }
            case 'Q':
            {
                if (InKeyEvent.GetInputEvent() == IE_Pressed)
                {
                    CameraInputPressedKeys.Add(EKeys::Q);
                }
                else if (InKeyEvent.GetInputEvent() == IE_Released)
                {
                    CameraInputPressedKeys.Remove(EKeys::Q);
                }
                break;
            }
            default:
                break;
            }
        }
        else
        {
            CameraInputPressedKeys.Empty();

            /* @todo 뷰포트와 플레이어 중 조작 모드 누가 관리할지 정하기
            // 마우스 우클릭 상태가 아닌 경우 - 변환 모드 설정
            UEditorPlayer* EdPlayer = CastChecked<UEditorEngine>(GEngine)->GetEditorPlayer();
            switch (InKeyEvent.GetCharacter())
            {
            case 'W':
            {
                if (InKeyEvent.GetInputEvent() == IE_Pressed)
                {
                    EdPlayer->SetMode(CM_TRANSLATION);
                }
                break;
            }
            case 'E':
            {
                if (InKeyEvent.GetInputEvent() == IE_Pressed)
                {
                    EdPlayer->SetMode(CM_ROTATION);
                }
                break;
            }
            case 'R':
            {
                if (InKeyEvent.GetInputEvent() == IE_Pressed)
                {
                    EdPlayer->SetMode(CM_SCALE);
                }
                break;
            }
            default:
                break;
            }
            */
        }


        // 일반적인 단일 키 이벤트
        if (InKeyEvent.GetInputEvent() == IE_Pressed)
        {
            switch (InKeyEvent.GetCharacter())
            {
            case 'F':
                {
                    TSet<AActor*> SelectedActors = GetWorld()->GetSelectedActors();
                    if (!SelectedActors.IsEmpty())
                    {
                        if (AActor* PickedActor = *SelectedActors.begin())
                        {
                            FViewportCameraTransform& ViewTransform = ViewTransformPerspective;
                            ViewTransform.SetLocation(
                                // TODO: 10.0f 대신, 정점의 min, max의 거리를 구해서 하면 좋을 듯
                                PickedActor->GetActorLocation() - (ViewTransform.GetForwardVector() * 10.0f)
                            );
                        }
                    }
                    break;
                }
            case 'M':
                {
                    if (UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
                    {
                        FEngineLoop::GraphicDevice.OnResize(OwnerWindow);
                        SLevelEditor* LevelEditor = EditorEngine->GetLevelEditor();
                        LevelEditor->SetEnableMultiViewport(AppWnd, !LevelEditor->IsMultiViewport(AppWnd));
                    }
                    break;
                }
            case 'D':
                {
                    if (PressedKeys.Contains(EKeys::LeftControl))
                    {
                        GetWorld()->DuplicateSelectedActors();
                    }
                }
            default:
                break;
            }

            // Virtual Key
            UEditorEngine* EditorEngine = CastChecked<UEditorEngine>(GEngine);
            switch (InKeyEvent.GetKeyCode())
            {
            case VK_DELETE:
                {
                    auto SelectedActors = GetWorld()->GetSelectedActors();
                    if (!SelectedActors.IsEmpty())
                    {
                        // @todo 선택된 Component가 있는 경우 Component를 지울 것
                        //if (SelectedActors.Num() == 1)
                        //{
                        //}
                        //else
                        //{
                            for (AActor* Actor : SelectedActors)
                            {
                                UE_LOG(LogLevel::Display, "Delete Actor - %s", *Actor->GetName());
                                Actor->Destroy();
                            }
                        //}
                    }
                    GetWorld()->ClearSelectedActors();
                    break;
                }
            case VK_SPACE:
            {
                EditorEngine->GetLevelEditor()->GetActiveViewportClientData().AddControlMode();
                break;
            }
            case VK_LCONTROL:
            {
                PressedKeys.Add(EKeys::LeftControl);
                break;
            }
            default:
                break;
            }
        }
    }
}

void FEditorViewportClient::MouseMove(const FPointerEvent& InMouseEvent)
{
    const auto& [DeltaX, DeltaY] = InMouseEvent.GetCursorDelta();

    // 마우스 우클릭이 되었을때만 실행되는 함수
    if (GetKeyState(VK_RBUTTON) & 0x8000)
    {
        // Yaw(좌우 회전) 및 Pitch(상하 회전) 값 변경
        if (IsPerspective())
        {
            CameraRotateYaw(DeltaX * 0.1f);  // X 이동에 따라 좌우 회전
            CameraRotatePitch(DeltaY * 0.1f);  // Y 이동에 따라 상하 회전
        }
        else
        {
            PivotMoveRight(DeltaX);
            PivotMoveUp(DeltaY);
        }
    }
}

void FEditorViewportClient::ResizeViewport(FRect Top, FRect Bottom, FRect Left, FRect Right)
{
    if (Viewport)
    { 
        Viewport->ResizeViewport(Top, Bottom, Left, Right);    
    }
    else
    {
        UE_LOG(LogLevel::Error, "Viewport is nullptr");
    }
    
    float Width = Viewport->GetFSlateRect().Width;
    float Height = Viewport->GetFSlateRect().Height;
    AspectRatio = Width / Height;

    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

void FEditorViewportClient::ResizeViewport(FRect InRect)
{
    if (Viewport)
    { 
        Viewport->ResizeViewport(InRect);    
    }
    else
    {
        UE_LOG(LogLevel::Error, "Viewport is nullptr");
    }
    
    float Width = Viewport->GetFSlateRect().Width;
    float Height = Viewport->GetFSlateRect().Height;
    AspectRatio = Width / Height;

    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

bool FEditorViewportClient::IsSelected(FVector2D Point)
{
    float TopLeftX = Viewport->GetFSlateRect().LeftTopX;
    float TopLeftY = Viewport->GetFSlateRect().LeftTopY;
    float Width = Viewport->GetFSlateRect().Width;
    float Height = Viewport->GetFSlateRect().Height;

    if (Point.X >= TopLeftX && Point.X <= TopLeftX + Width &&
        Point.Y >= TopLeftY && Point.Y <= TopLeftY + Height)
    {
        return true;
    }
    return false;
}

const D3D11_VIEWPORT& FEditorViewportClient::GetD3DViewport()
{
    return Viewport->GetViewport();
}

void FEditorViewportClient::CalculateCascadeSplits(float NearClip, float FarClip)
{
    // 지수 분할 방식
    float lambda = 0.75f;  // 지수 분할 계수 (0: 균등 분할, 1: 완전 지수)
    
    for (int i = 0; i < CASCADE_COUNT; i++) {
        float p = (i + 1) / float(CASCADE_COUNT);
        
        // 균등 분할 값
        float uniformSplit = NearClip + (FarClip - NearClip) * p;
        
        // 지수 분할 값 (로그 스케일)
        float exponentialSplit = NearClip * pow(FarClip / NearClip, p);
        
        // 람다를 사용하여 두 방식을 혼합
        cascadeSplits[i] = lambda * exponentialSplit + (1 - lambda) * uniformSplit;
    }
}

void FEditorViewportClient::CalculateFrustumCorners(UINT cascadeIndex)
{
    float prevSplitDist = cascadeIndex == 0 ? GetNearClip() : cascadeSplits[cascadeIndex - 1];
    float splitDist = cascadeSplits[cascadeIndex];
    
    // 카메라 공간에서 절두체 코너 계산
    // 근평면 4개 코너 + 원평면 4개 코너
    CalculateFrustumCornersInCameraSpace(prevSplitDist, splitDist, cascadeIndex);
    
    // 월드 공간으로 변환
    FMatrix cameraToWorldMatrix = FMatrix::Inverse(GetViewMatrix());
    for (int i = 0; i < 8; i++) {
        FVector4 worldCorner = cameraToWorldMatrix.TransformFVector4(FVector4(cascadeCorners[cascadeIndex][i], 1.0f));
        cascadeCorners[cascadeIndex][i] = FVector(worldCorner.X, worldCorner.Y, worldCorner.Z);
    }
}

void FEditorViewportClient::CalculateFrustumCornersInCameraSpace(float NearDist, float FarDist, int CascadeIndex) {
    // 카메라 투영 행렬에서 필요한 값들을 추출
    float fov = GetViewFOV();  // 시야각(라디안)
    fov = FMath::DegreesToRadians(fov);
    
    // 근평면과 원평면의 높이와 너비 계산
    float nearHeight = 2.0f * tan(fov * 0.5f) * NearDist;
    float nearWidth = nearHeight * AspectRatio;
    float farHeight = 2.0f * tan(fov * 0.5f) * FarDist;
    float farWidth = farHeight * AspectRatio;
    
    // 근평면의 4개 모서리 계산 (카메라 공간)
    // 왼쪽 아래
    cascadeCorners[CascadeIndex][0] = FVector(-nearWidth * 0.5f, -nearHeight * 0.5f, NearDist);
    // 오른쪽 아래
    cascadeCorners[CascadeIndex][1] = FVector(nearWidth * 0.5f, -nearHeight * 0.5f, NearDist);
    // 오른쪽 위
    cascadeCorners[CascadeIndex][2] = FVector(nearWidth * 0.5f, nearHeight * 0.5f, NearDist);
    // 왼쪽 위
    cascadeCorners[CascadeIndex][3] = FVector(-nearWidth * 0.5f, nearHeight * 0.5f, NearDist);
    
    // 원평면의 4개 모서리 계산 (카메라 공간)
    // 왼쪽 아래
    cascadeCorners[CascadeIndex][4] = FVector(-farWidth * 0.5f, -farHeight * 0.5f, FarDist);
    // 오른쪽 아래
    cascadeCorners[CascadeIndex][5] = FVector(farWidth * 0.5f, -farHeight * 0.5f, FarDist);
    // 오른쪽 위
    cascadeCorners[CascadeIndex][6] = FVector(farWidth * 0.5f, farHeight * 0.5f, FarDist);
    // 왼쪽 위
    cascadeCorners[CascadeIndex][7] = FVector(-farWidth * 0.5f, farHeight * 0.5f, FarDist);
}

void FEditorViewportClient::UpdateCascadeShadowArea()
{
    CalculateCascadeSplits(GetNearClip(), GetFarClip());
    for (int i=0;i<CASCADE_COUNT;i++)
    {
        CalculateFrustumCorners(i);
    }
}

void FEditorViewportClient::CameraMoveForward(float _Value)
{
    if (!OverrideComponent)
    {
        if (IsPerspective()) {
            FVector curCameraLoc = ViewTransformPerspective.GetLocation();
            curCameraLoc = curCameraLoc + ViewTransformPerspective.GetForwardVector() * GetCameraSpeedScalar() * _Value;
            ViewTransformPerspective.SetLocation(curCameraLoc);
        }
        else
        {
            Pivot.X += _Value * 0.1f;
        }
    }
}

void FEditorViewportClient::CameraMoveRight(float _Value)
{
    if (!OverrideComponent)
    {
        if (IsPerspective()) {
            FVector curCameraLoc = ViewTransformPerspective.GetLocation();
            curCameraLoc = curCameraLoc + ViewTransformPerspective.GetRightVector() * GetCameraSpeedScalar() * _Value;
            ViewTransformPerspective.SetLocation(curCameraLoc);
        }
        else
        {
            Pivot.Y += _Value * 0.1f;
        }
    }
}

void FEditorViewportClient::CameraMoveUp(float _Value)
{
    if (!OverrideComponent)
    {
        if (IsPerspective()) {
            FVector curCameraLoc = ViewTransformPerspective.GetLocation();
            curCameraLoc.Z = curCameraLoc.Z + GetCameraSpeedScalar() * _Value;
            ViewTransformPerspective.SetLocation(curCameraLoc);
        }
        else {
            Pivot.Z += _Value * 0.1f;
        }
    }
}

void FEditorViewportClient::CameraRotateYaw(float _Value)
{
    if (!OverrideComponent)
    {
        FVector curCameraRot = ViewTransformPerspective.GetRotation();
        curCameraRot.Z += _Value;
        ViewTransformPerspective.SetRotation(curCameraRot);
    }
}

void FEditorViewportClient::CameraRotatePitch(float _Value)
{
    if (!OverrideComponent)
    {
        FVector curCameraRot = ViewTransformPerspective.GetRotation();
        curCameraRot.Y += _Value;
        if (curCameraRot.Y < -89.0f)
            curCameraRot.Y = -89.0f;
        if (curCameraRot.Y > 89.0f)
            curCameraRot.Y = 89.0f;
        ViewTransformPerspective.SetRotation(curCameraRot);
    }
}

void FEditorViewportClient::PivotMoveRight(float _Value)
{
    if (!OverrideComponent)
    {
        Pivot = Pivot + ViewTransformOrthographic.GetRightVector() * _Value * -0.05f;
    }
}

void FEditorViewportClient::PivotMoveUp(float _Value)
{
    if (!OverrideComponent)
    {
        Pivot = Pivot + ViewTransformOrthographic.GetUpVector() * _Value * 0.05f;
    }
}

void FEditorViewportClient::UpdateViewMatrix()
{
    if (!OverrideComponent)
    {
        if (IsPerspective()) {
            nearPlane = 0.1f;
            farPlane = 1000.f;
            View = JungleMath::CreateViewMatrix(ViewTransformPerspective.GetLocation(),
                ViewTransformPerspective.GetLocation() + ViewTransformPerspective.GetForwardVector(),
                FVector{ 0.0f,0.0f, 1.0f });
        }
        else
        {
            UpdateOrthoCameraLoc();
            if (ViewportType == LVT_OrthoXY || ViewportType == LVT_OrthoNegativeXY) {
                View = JungleMath::CreateViewMatrix(ViewTransformOrthographic.GetLocation(),
                    Pivot, FVector(0.0f, -1.0f, 0.0f));
            }
            else
            {
                View = JungleMath::CreateViewMatrix(ViewTransformOrthographic.GetLocation(),
                    Pivot, FVector(0.0f, 0.0f, 1.0f));
            }
        }
    }
    else
    {
        if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(OverrideComponent))
        {
            View = SpotLight->GetViewMatrix();
        }
        if (UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(OverrideComponent))
        {
            View = DirectionalLight->GetViewMatrix();
        }
        if (UCameraComponent* PlayerCamera = Cast<UCameraComponent>(OverrideComponent))
        {
            View = PlayerCamera->GetViewMatrix();
        }
    }
}

void FEditorViewportClient::UpdateProjectionMatrix()
{
    if (!OverrideComponent)
    {
        if (IsPerspective()) {
            Projection = JungleMath::CreateProjectionMatrix(
                ViewFOV * (3.141592f / 180.0f),
                GetViewport()->GetFSlateRect().Width / GetViewport()->GetFSlateRect().Height,
                nearPlane,
                farPlane
            );
        }
        else
        {
            // 오쏘그래픽 너비는 줌 값과 가로세로 비율에 따라 결정됩니다.
            float orthoWidth = OrthoSize * AspectRatio;
            float orthoHeight = OrthoSize;

            // 오쏘그래픽 투영 행렬 생성 (nearPlane, farPlane 은 기존 값 사용)
            Projection = JungleMath::CreateOrthoProjectionMatrix(
                orthoWidth,
                orthoHeight,
                nearPlane,
                farPlane
            );
        }
    }
    else
    {
        if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(OverrideComponent))
        {
            Projection = SpotLight->GetProjectionMatrix();
        }
        if (UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(OverrideComponent))
        {
            Projection = DirectionalLight->GetProjectionMatrix();
        }
        if (UCameraComponent* PlayerCamera = Cast<UCameraComponent>(OverrideComponent))
        {
            Projection = JungleMath::CreateProjectionMatrix(
                    FMath::DegreesToRadians(PlayerCamera->GetFOV()),
                    AspectRatio,
                    PlayerCamera->GetNearClip(),
                    PlayerCamera->GetFarClip()
                );
        }
    }
}

bool FEditorViewportClient::IsOrtho() const
{
    return !IsPerspective();
}

bool FEditorViewportClient::IsPerspective() const
{
    return (GetViewportType() == LVT_Perspective);
}

ELevelViewportType FEditorViewportClient::GetViewportType() const
{
    ELevelViewportType EffectiveViewportType = ViewportType;
    if (EffectiveViewportType == LVT_None)
    {
        EffectiveViewportType = LVT_Perspective;
    }
    //if (bUseControllingActorViewInfo)
    //{
    //    EffectiveViewportType = (ControllingActorViewInfo.ProjectionMode == ECameraProjectionMode::Perspective) ? LVT_Perspective : LVT_OrthoFreelook;
    //}
    return EffectiveViewportType;
}

void FEditorViewportClient::SetViewportType(ELevelViewportType InViewportType)
{
    ViewportType = InViewportType;
    //ApplyViewMode(GetViewMode(), IsPerspective(), EngineShowFlags);

    //// We might have changed to an orthographic viewport; if so, update any viewport links
    //UpdateLinkedOrthoViewports(true);

    //Invalidate();
}

void FEditorViewportClient::UpdateOrthoCameraLoc()
{
    const float farDistance = farPlane * 0.5f;
    switch (ViewportType)
    {
    case LVT_OrthoXY: // Top
        ViewTransformOrthographic.SetLocation(Pivot + FVector::UpVector*farDistance);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, 90.0f, -90.0f));
        break;
    case LVT_OrthoXZ: // Front
        ViewTransformOrthographic.SetLocation(Pivot + FVector::ForwardVector*farDistance);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, 0.0f, 180.0f));
        break;
    case LVT_OrthoYZ: // Left
        ViewTransformOrthographic.SetLocation(Pivot + FVector::RightVector*farDistance);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, 0.0f, 270.0f));
        break;
    case LVT_Perspective:
        break;
    case LVT_OrthoNegativeXY: // Bottom
        ViewTransformOrthographic.SetLocation(Pivot + FVector::UpVector * -1.0f*farDistance);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, -90.0f, 90.0f));
        break;
    case LVT_OrthoNegativeXZ: // Back
        ViewTransformOrthographic.SetLocation(Pivot + FVector::ForwardVector * -1.0f*farDistance);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, 0.0f, 0.0f));
        break;
    case LVT_OrthoNegativeYZ: // Right
        ViewTransformOrthographic.SetLocation(Pivot + FVector::RightVector * -1.0f*farDistance);
        ViewTransformOrthographic.SetRotation(FVector(0.0f, 0.0f, 90.0f));
        break;
    case LVT_MAX:
        break;
    case LVT_None:
        break;
    default:
        break;
    }
}

void FEditorViewportClient::SetOrthoSize(float InValue)
{
    OrthoSize = InValue;
    OrthoSize = FMath::Max(OrthoSize, 0.1f);
}

void FEditorViewportClient::LoadConfig(const TMap<FString, FString>& config)
{
    FString ViewportNum = std::to_string(ViewportIndex);
    CameraSpeedSetting = GetValueFromConfig(config, "CameraSpeedSetting" + ViewportNum, 1);
    CameraSpeedScalar = GetValueFromConfig(config, "CameraSpeedScalar" + ViewportNum, 1.0f);
    GridSize = GetValueFromConfig(config, "GridSize"+ ViewportNum, 10.0f);
    ViewTransformPerspective.ViewLocation.X = GetValueFromConfig(config, "PerspectiveCameraLocX" + ViewportNum, 0.0f);
    ViewTransformPerspective.ViewLocation.Y = GetValueFromConfig(config, "PerspectiveCameraLocY" + ViewportNum, 0.0f);
    ViewTransformPerspective.ViewLocation.Z = GetValueFromConfig(config, "PerspectiveCameraLocZ" + ViewportNum, 0.0f);
    ViewTransformPerspective.ViewRotation.X = GetValueFromConfig(config, "PerspectiveCameraRotX" + ViewportNum, 0.0f);
    ViewTransformPerspective.ViewRotation.Y = GetValueFromConfig(config, "PerspectiveCameraRotY" + ViewportNum, 0.0f);
    ViewTransformPerspective.ViewRotation.Z = GetValueFromConfig(config, "PerspectiveCameraRotZ" + ViewportNum, 0.0f);
    ShowFlag = GetValueFromConfig(config, "ShowFlag" + ViewportNum, 31.0f);
    ViewMode = static_cast<EViewModeIndex>(GetValueFromConfig(config, "ViewMode" + ViewportNum, 0));
    ViewportType = static_cast<ELevelViewportType>(GetValueFromConfig(config, "ViewportType" + ViewportNum, 3));
}
void FEditorViewportClient::SaveConfig(TMap<FString, FString>& config)
{
    FString ViewportNum = std::to_string(ViewportIndex);
    config["CameraSpeedSetting"+ ViewportNum] = std::to_string(CameraSpeedSetting);
    config["CameraSpeedScalar"+ ViewportNum] = std::to_string(CameraSpeedScalar);
    config["GridSize"+ ViewportNum] = std::to_string(GridSize);
    config["PerspectiveCameraLocX" + ViewportNum] = std::to_string(ViewTransformPerspective.GetLocation().X);
    config["PerspectiveCameraLocY" + ViewportNum] = std::to_string(ViewTransformPerspective.GetLocation().Y);
    config["PerspectiveCameraLocZ" + ViewportNum] = std::to_string(ViewTransformPerspective.GetLocation().Z);
    config["PerspectiveCameraRotX" + ViewportNum] = std::to_string(ViewTransformPerspective.GetRotation().X);
    config["PerspectiveCameraRotY" + ViewportNum] = std::to_string(ViewTransformPerspective.GetRotation().Y);
    config["PerspectiveCameraRotZ" + ViewportNum] = std::to_string(ViewTransformPerspective.GetRotation().Z);
    config["ShowFlag"+ ViewportNum] = std::to_string(ShowFlag);
    config["ViewMode" + ViewportNum] = std::to_string(int32(ViewMode));
    config["ViewportType" + ViewportNum] = std::to_string(int32(ViewportType));
}
TMap<FString, FString> FEditorViewportClient::ReadIniFile(const FString& filePath)
{
    TMap<FString, FString> config;
    std::ifstream file(*filePath);
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[' || line[0] == ';') continue;
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            config[key] = value;
        }
    }
    return config;
}

void FEditorViewportClient::WriteIniFile(const FString& filePath, const TMap<FString, FString>& config)
{
    std::ofstream file(*filePath);
    for (const auto& pair : config) {
        file << *pair.Key << "=" << *pair.Value << "\n";
    }
}

void FEditorViewportClient::SetCameraSpeed(float value)
{
    CameraSpeedScalar = FMath::Clamp(value, 0.1f, 200.0f);
}


FVector FViewportCameraTransform::GetForwardVector()
{
    FVector Forward = FVector::ForwardVector;
    Forward = JungleMath::FVectorRotate(Forward, ViewRotation);
    return Forward;
}
FVector FViewportCameraTransform::GetRightVector()
{
    FVector Right = FVector::RightVector;
	Right = JungleMath::FVectorRotate(Right, ViewRotation);
	return Right;
}

FVector FViewportCameraTransform::GetUpVector()
{
    FVector Up = FVector::UpVector;
    Up = JungleMath::FVectorRotate(Up, ViewRotation);
    return Up;
}

FViewportCameraTransform::FViewportCameraTransform()
{
}
