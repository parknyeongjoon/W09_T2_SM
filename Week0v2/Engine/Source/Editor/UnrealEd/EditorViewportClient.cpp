#include "EditorViewportClient.h"
#include "fstream"
#include "sstream"
#include "ostream"
#include "Math/JungleMath.h"
#include "EditorEngine.h"
#include "UnrealClient.h"
#include "Engine/FLoaderOBJ.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Engine/Classes/Engine/StaticMeshActor.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
FVector FEditorViewportClient::Pivot = FVector(0.0f, 0.0f, 0.0f);
float FEditorViewportClient::orthoSize = 10.0f;
FEditorViewportClient::FEditorViewportClient()
    : Viewport(nullptr), ViewMode(VMI_Lit_Phong), ViewportType(LVT_Perspective), ShowFlag(31)
{

}

FEditorViewportClient::~FEditorViewportClient()
{
    Release();
}

void FEditorViewportClient::Draw(FViewport* Viewport)
{
}

void FEditorViewportClient::Initialize(int32 viewportIndex)
{

    ViewTransformPerspective.SetLocation(FVector(8.0f, 8.0f, 8.f));
    ViewTransformPerspective.SetRotation(FVector(0.0f, 45.0f, -135.0f));
    Viewport = new FViewport(static_cast<EViewScreenLocation>(viewportIndex));
    ResizeViewport(GEngine->graphicDevice.SwapchainDesc);
    ViewportIndex = viewportIndex;
}

void FEditorViewportClient::Tick(float DeltaTime)
{
    Input();
    UpdateViewMatrix();
    UpdateProjectionMatrix();
    UpdateCascadeShadowArea();

    // ----- Test Start
    // if (DebugCube.IsEmpty())
    // {
    //     for (int i=0;i<CASCADE_COUNT*8;i++)
    //     {
    //         AStaticMeshActor* TempActor = GEngine->GetWorld()->SpawnActor<AStaticMeshActor>();
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
    if (Viewport)
        delete Viewport;
 
}



void FEditorViewportClient::Input()
{
    if (GEngine->levelType != LEVELTICK_ViewportsOnly) return;
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) // VK_RBUTTON은 마우스 오른쪽 버튼을 나타냄
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        if (!bRightMouseDown)
        {
            // 마우스 오른쪽 버튼을 처음 눌렀을 때, 마우스 위치 초기화
            GetCursorPos(&lastMousePos);
            bRightMouseDown = true;
        }
        else
        {
            // 마우스 이동량 계산
            POINT currentMousePos;
            GetCursorPos(&currentMousePos);

            // 마우스 이동 차이 계산
            int32 deltaX = currentMousePos.x - lastMousePos.x;
            int32 deltaY = currentMousePos.y - lastMousePos.y;

            // Yaw(좌우 회전) 및 Pitch(상하 회전) 값 변경
            if (IsPerspective()) {
                CameraRotateYaw(deltaX * 0.1f);  // X 이동에 따라 좌우 회전
                CameraRotatePitch(deltaY * 0.1f);  // Y 이동에 따라 상하 회전
            }
            else
            {
                PivotMoveRight(deltaX);
                PivotMoveUp(deltaY);
            }

            SetCursorPos(lastMousePos.x, lastMousePos.y);
        }
        if (!bLCtrlDown)
        {
            if (GetAsyncKeyState('A') & 0x8000)
            {
                CameraMoveRight(-1.f);
            }
            if (GetAsyncKeyState('D') & 0x8000)
            {
                CameraMoveRight(1.f);
            }
            if (GetAsyncKeyState('W') & 0x8000)
            {
                CameraMoveForward(1.f);
            }
            if (GetAsyncKeyState('S') & 0x8000)
            {
                CameraMoveForward(-1.f);
            }
            if (GetAsyncKeyState('E') & 0x8000)
            {
                CameraMoveUp(1.f);
            }
            if (GetAsyncKeyState('Q') & 0x8000)
            {
                CameraMoveUp(-1.f);
            }
        }
    }
    else
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        bRightMouseDown = false; // 마우스 오른쪽 버튼을 떼면 상태 초기화
    }

    // Focus Selected Actor
    if (GetAsyncKeyState('F') & 0x8000)
    {
        if (!GEngine->GetWorld()->GetSelectedActors().IsEmpty())
        {
            AActor* PickedActor = *GEngine->GetWorld()->GetSelectedActors().begin();
            FViewportCameraTransform& ViewTransform = ViewTransformPerspective;
            ViewTransform.SetLocation(
                // TODO: 10.0f 대신, 정점의 min, max의 거리를 구해서 하면 좋을 듯
                PickedActor->GetActorLocation() - (ViewTransform.GetForwardVector() * 10.0f)
            );
        }
    }
    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
    {
        if (!bLCtrlDown)
            bLCtrlDown = true;
    }
    else
    {
        bLCtrlDown = false;
    }
}
void FEditorViewportClient::ResizeViewport(const DXGI_SWAP_CHAIN_DESC& swapchaindesc)
{
    if (Viewport) { 
        Viewport->ResizeViewport(swapchaindesc);    
    }
    else {
        UE_LOG(LogLevel::Error, "Viewport is nullptr");
    }
    AspectRatio = GEngine->GetAspectRatio(GEngine->graphicDevice.SwapChain);
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}
void FEditorViewportClient::ResizeViewport(FRect Top, FRect Bottom, FRect Left, FRect Right)
{
    if (Viewport) {
        Viewport->ResizeViewport(Top, Bottom, Left, Right);
    }
    else {
        UE_LOG(LogLevel::Error, "Viewport is nullptr");
    }
    AspectRatio = GEngine->GetAspectRatio(GEngine->graphicDevice.SwapChain);
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}
bool FEditorViewportClient::IsSelected(POINT point)
{
    float TopLeftX = Viewport->GetScreenRect().TopLeftX;
    float TopLeftY = Viewport->GetScreenRect().TopLeftY;
    float Width = Viewport->GetScreenRect().Width;
    float Height = Viewport->GetScreenRect().Height;

    if (point.x >= TopLeftX && point.x <= TopLeftX + Width &&
        point.y >= TopLeftY && point.y <= TopLeftY + Height)
    {
        return true;
    }
    return false;
}
D3D11_VIEWPORT& FEditorViewportClient::GetD3DViewport()
{
    return Viewport->GetScreenRect();
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
        cascadeCorners[cascadeIndex][i] = FVector(worldCorner.x, worldCorner.y, worldCorner.z);
    }
}

void FEditorViewportClient::CalculateFrustumCornersInCameraSpace(float NearDist, float FarDist, int CascadeIndex) {
    // 카메라 투영 행렬에서 필요한 값들을 추출
    float fov = GetViewFOV();  // 시야각(라디안)
    fov = FMath::DegreesToRadians(fov);
    float aspect = GetAspectRatio();  // 종횡비(width/height)
    
    // 근평면과 원평면의 높이와 너비 계산
    float nearHeight = 2.0f * tan(fov * 0.5f) * NearDist;
    float nearWidth = nearHeight * aspect;
    float farHeight = 2.0f * tan(fov * 0.5f) * FarDist;
    float farWidth = farHeight * aspect;
    
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
                GetViewport()->GetScreenRect().Width / GetViewport()->GetScreenRect().Height,
                nearPlane,
                farPlane
            );
        }
        else
        {
            // 스왑체인의 가로세로 비율을 구합니다.
            float aspectRatio = GetViewport()->GetScreenRect().Width / GetViewport()->GetScreenRect().Height;

            // 오쏘그래픽 너비는 줌 값과 가로세로 비율에 따라 결정됩니다.
            float orthoWidth = orthoSize * aspectRatio;
            float orthoHeight = orthoSize;

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
            Projection = PlayerCamera->GetProjectionMatrix();
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

void FEditorViewportClient::SetOthoSize(float _Value)
{
    orthoSize += _Value;
    if (orthoSize <= 0.1f)
        orthoSize = 0.1f;
    
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

void FEditorViewportClient::SetCameraSpeedScalar(float value)
{
    if (value < 0.198f)
        value = 0.198f;
    else if (value > 176.0f)
        value = 176.0f;
    CameraSpeedScalar = value;
}


FVector FViewportCameraTransform::GetForwardVector()
{
    FVector Forward = FVector(1.f, 0.f, 0.0f);
    Forward = JungleMath::FVectorRotate(Forward, ViewRotation);
    return Forward;
}
FVector FViewportCameraTransform::GetRightVector()
{
    FVector Right = FVector(0.f, 1.f, 0.0f);
	Right = JungleMath::FVectorRotate(Right, ViewRotation);
	return Right;
}

FVector FViewportCameraTransform::GetUpVector()
{
    FVector Up = FVector(0.f, 0.f, 1.0f);
    Up = JungleMath::FVectorRotate(Up, ViewRotation);
    return Up;
}

FViewportCameraTransform::FViewportCameraTransform()
{
}
