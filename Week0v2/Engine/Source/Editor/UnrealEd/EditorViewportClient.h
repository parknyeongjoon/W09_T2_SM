#pragma once
#include <SlateCore/Input/Events.h>

#include "Define.h"
#include "EngineBaseTypes.h"
#include "ViewportClient.h"
#include "Container/Map.h"
#include "Container/String.h"
#include "HAL/PlatformType.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "UObject/ObjectMacros.h"

#define MIN_ORTHOZOOM				1.0							/* 2D ortho viewport zoom >= MIN_ORTHOZOOM */
#define MAX_ORTHOZOOM				1e25	

class FRect;
class AActor;
class USceneComponent;

struct FViewportCameraTransform
{
public:
    FVector GetForwardVector();
    FVector GetRightVector();
    FVector GetUpVector();

public:
    FViewportCameraTransform();

    /** Sets the transform's location */
    void SetLocation(const FVector& Position)
    {
        ViewLocation = Position;
    }

    /** Sets the transform's rotation */
    void SetRotation(const FVector& Rotation)
    {
        ViewRotation = Rotation;
    }

    /** Sets the location to look at during orbit */
    void SetLookAt(const FVector& InLookAt)
    {
        LookAt = InLookAt;
    }

    /** Set the ortho zoom amount */
    void SetOrthoZoom(float InOrthoZoom)
    {
        assert(InOrthoZoom >= MIN_ORTHOZOOM && InOrthoZoom <= MAX_ORTHOZOOM);
        OrthoZoom = InOrthoZoom;
    }

    /** Check if transition curve is playing. */
 /*    bool IsPlaying();*/

    /** @return The transform's location */
    FORCEINLINE const FVector& GetLocation() const { return ViewLocation; }

    /** @return The transform's rotation */
    FORCEINLINE const FVector& GetRotation() const { return ViewRotation; }

    /** @return The look at point for orbiting */
    FORCEINLINE const FVector& GetLookAt() const { return LookAt; }

    /** @return The ortho zoom amount */
    FORCEINLINE float GetOrthoZoom() const { return OrthoZoom; }

public:
    /** Current viewport Position. */
    FVector	ViewLocation;
    /** Current Viewport orientation; valid only for perspective projections. */
    FVector ViewRotation;
    FVector	DesiredLocation;
    /** When orbiting, the point we are looking at */
    FVector LookAt;
    /** Viewport start location when animating to another location */
    FVector StartLocation;
    /** Ortho zoom amount */
    float OrthoZoom;
};

class FEditorViewportClient : public FViewportClient
{
public:
    FEditorViewportClient();
    ~FEditorViewportClient();

    virtual void        Draw(FViewport* Viewport) override;
    UWorld*             GetWorld() const override;
    virtual void        Initialize(HWND InOwnerWindow, uint32 InViewportIndex, UWorld* World) override;
    
    void Tick(float DeltaTime);
    void Release();
    void UpdateEditorCameraMovement(float DeltaTime);

    // @todo MultiViewport 떄문에 AppWnd를 인자로 받는중이다. 제거할 방안
    void InputKey(HWND AppWnd, const FKeyEvent& InKeyEvent);
    void MouseMove(const FPointerEvent& InMouseEvent);

    void ResizeViewport(FRect Top, FRect Bottom, FRect Left, FRect Right);
    void ResizeViewport(FRect InRect);

    bool IsSelected(FVector2D Point);

protected:
    /** Camera speed setting */
    int32 CameraSpeedSetting = 1;
    /** Camera speed scalar */

public:
    float CameraSpeedScalar = 1.0f;
    float GridSize;

public: 
    FViewport* GetViewport() const { return Viewport; }

    const D3D11_VIEWPORT& GetD3DViewport();
    
    uint32 GetViewportIndex() const { return ViewportIndex; }
    void SetViewportIndex(uint32 InIndex) { ViewportIndex = InIndex; }
    
private:
    FViewport* Viewport;
    uint32 ViewportIndex;
    
public:
    //카메라
    /** Viewport camera transform data for perspective viewports */
    FViewportCameraTransform		ViewTransformPerspective;
    FViewportCameraTransform        ViewTransformOrthographic;
    //override camera
    USceneComponent* OverrideComponent = nullptr;
    // 카메라 정보 
    float ViewFOV = 60.0f;
    /** Viewport's stored horizontal field of view (saved in ini files). */
    float FOVAngle = 60.0f;
    float AspectRatio;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    static FVector Pivot;
    static float OrthoSize;
    
    ELevelViewportType ViewportType;
    uint64 ShowFlag;
    EViewModeIndex ViewMode;

    FMatrix View;
    FMatrix Projection;

    // Cascade Shadow Map
private:
    float cascadeSplits[CASCADE_COUNT]; // 카스케이드 경계 저장
    FVector cascadeCorners[CASCADE_COUNT][8];

    
    void CalculateCascadeSplits(float NearClip, float FarClip);
    void CalculateFrustumCorners(UINT CascadeIndex);
    void CalculateFrustumCornersInCameraSpace(float NearDist, float FarDist, int CascadeIndex);


    //Test
    TArray<AActor*> DebugCube;

public: //Camera Movement
    FVector* GetCascadeCorner(UINT CascadeIndex) { return cascadeCorners[CascadeIndex]; }
    float GetCascadeSplit(UINT CascadeIndex) const { return cascadeSplits[CascadeIndex]; }
    void UpdateCascadeShadowArea();
    void CameraMoveForward(float _Value);
    void CameraMoveRight(float _Value);
    void CameraMoveUp(float _Value);
    void CameraRotateYaw(float _Value);
    void CameraRotatePitch(float _Value);
    void PivotMoveRight(float _Value);
    void PivotMoveUp(float _Value);

    FMatrix& GetViewMatrix() { return  View; }
    FMatrix& GetProjectionMatrix() { return Projection; }
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    void SetViewFOV(float viewFOV) {ViewFOV = viewFOV;}
    float GetViewFOV() {return ViewFOV;}
    void SetNearClip(float newNearClip) { nearPlane = newNearClip; }
    float GetNearClip() {return nearPlane;}
    void SetFarClip(float newFarClip) {farPlane = newFarClip;}
    float GetFarClip() {return farPlane;}
    float GetAspectRatio() { return AspectRatio; }
    
    bool IsOrtho() const;
    bool IsPerspective() const;
    ELevelViewportType GetViewportType() const;
    void SetViewportType(ELevelViewportType InViewportType);
    void UpdateOrthoCameraLoc();
    EViewModeIndex GetViewMode() const { return ViewMode; }
    void SetViewMode(const EViewModeIndex newMode) { ViewMode = newMode; }
    uint64 GetShowFlag() { return ShowFlag; }
    void SetShowFlag(uint64 newMode) { ShowFlag = newMode; }
    bool GetIsOnRBMouseClick() { return bRightMouseDown; }

    //camera overriding
    USceneComponent* GetOverrideComponent() { return OverrideComponent; }
    void SetOverrideComponent(USceneComponent* newComp) { OverrideComponent = newComp; }

    static float GetOrthoSize() { return OrthoSize; }
    static void SetOrthoSize(float InValue);
private: // Input
    POINT lastMousePos;
    bool bRightMouseDown = false;
    bool bLCtrlDown = false;

    //Temp
    TSet<EKeys::Type> CameraInputPressedKeys;

    TSet<EKeys::Type> PressedKeys;
    
public:
    void LoadConfig(const TMap<FString, FString>& config);
    void SaveConfig(TMap<FString, FString>& config);
private:
    TMap<FString, FString> ReadIniFile(const FString& filePath);
    void WriteIniFile(const FString& filePath, const TMap<FString, FString>& config);
	
public:
    float GetCameraSpeedScalar() const { return CameraSpeedScalar; };
    void SetCameraSpeed(float value);

private:
    template <typename T>
    T GetValueFromConfig(const TMap<FString, FString>& config, const FString& key, T defaultValue) {
        if (const FString* Value = config.Find(key))
        {
            std::istringstream iss(**Value);
            T value;
            if (iss >> value)
            {
                return value;
            }
        }
        return defaultValue;
    }
};

