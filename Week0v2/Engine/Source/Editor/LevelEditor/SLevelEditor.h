#pragma once
#include "Delegates/Delegate.h"
#include "Math/Vector.h"
#include "Runtime/Engine/Classes/Engine/FEditorStateManager.h"
#include "UObject/ObjectTypes.h"

class FViewportClient;
class SSplitterH;
class SSplitterV;
class FEditorViewportClient;

struct FWindowViewportClientData
{
public:    
    float EditorWidth;
    float EditorHeight;

    bool bMultiViewportMode;
    uint32 ActiveViewportIndex;

    ControlMode cMode;
    CoordiMode cdMode;

    TArray<std::shared_ptr<FEditorViewportClient>> ViewportClients;
    
    std::shared_ptr<SSplitterH> HSplitter;
    std::shared_ptr<SSplitterV> VSplitter;

public:
    void SetMode(ControlMode _Mode) { cMode = _Mode; }
    ControlMode GetControlMode() const { return cMode; }
    CoordiMode GetCoordiMode() const { return cdMode; }
    void AddControlMode() { cMode = static_cast<ControlMode>((cMode + 1) % CM_END); }
    void AddCoordiMode() { cdMode = static_cast<CoordiMode>((cdMode + 1) % CDM_END); }
    
    std::shared_ptr<FEditorViewportClient> GetActiveViewportClient() { return ViewportClients[ActiveViewportIndex]; }

    FWindowViewportClientData()
        : EditorWidth(0.0f)
        , EditorHeight(0.0f)
        , bMultiViewportMode(false)
        , ActiveViewportIndex(0)
        , cMode()
        , cdMode()
    {
    }
};

class SLevelEditor
{
public:
    SLevelEditor();
    ~SLevelEditor() = default;

    void Initialize(UWorld* World, HWND OwnerWindow);
    void Tick(double DeltaTime);
    void Release();
    
    template <typename T>
        requires std::derived_from<T, FViewportClient>
    std::shared_ptr<T> AddViewportClient(HWND OwnerWindow, UWorld* World);

    void RemoveViewportClient(HWND OwnerWindow, std::shared_ptr<FEditorViewportClient> ViewportClient);
    void RemoveViewportClients(HWND HWnd);

    void ResizeWindow(HWND AppWnd, FVector2D ClientSize);
    void ResizeViewports(HWND AppWnd);

    void SelectViewport(HWND AppWnd, FVector2D Point);
    void SetEnableMultiViewport(HWND AppWnd, bool bIsEnable);
    bool IsMultiViewport(HWND AppWnd);

    void RegisterEditorInputDelegates();
    void RegisterPIEInputDelegates();

private:
    // TArray<std::shared_ptr<FEditorViewportClient>> ViewportClients;

    uint32 ActiveViewportClientIndex = 0;
    HWND ActiveViewportWindow = nullptr;

    /** 우클릭 시 캡처된 마우스 커서의 초기 위치 (스크린 좌표계) */
    FVector2D MousePinPosition;

    TArray<FDelegateHandle> InputDelegatesHandles;
    
    FEditorStateManager EditorStateManager;

public:
    TMap<HWND, FWindowViewportClientData> WindowViewportDataMap;
public:

    uint32 GetCurrentViewportClientIndex() const { return ActiveViewportClientIndex; }
    HWND GetCurrentViewportWindow() const { return ActiveViewportWindow; }

    TArray<std::shared_ptr<FEditorViewportClient>> GetViewportClients(HWND AppWnd) const
    {
        if (!WindowViewportDataMap.Contains(ActiveViewportWindow))
        {
            return TArray<std::shared_ptr<FEditorViewportClient>>();
        }
        
        return WindowViewportDataMap[AppWnd].ViewportClients;
    }
    
    std::shared_ptr<FEditorViewportClient> GetActiveViewportClient() const
    {
        if (!WindowViewportDataMap.Contains(ActiveViewportWindow))
        {
            return nullptr;
        }
        return WindowViewportDataMap[ActiveViewportWindow].ViewportClients[ActiveViewportClientIndex];
    }

    FWindowViewportClientData& GetActiveViewportClientData()
    {
        if (!WindowViewportDataMap.Contains(ActiveViewportWindow))
        {
            FWindowViewportClientData temp = FWindowViewportClientData();
            return temp;
        }
        return WindowViewportDataMap[ActiveViewportWindow];
    }

    FWindowViewportClientData& GetViewportClientData(HWND HWnd)
    {
        if (!WindowViewportDataMap.Contains(HWnd))
        {
            FWindowViewportClientData temp = FWindowViewportClientData();
            return temp;
        }
        return WindowViewportDataMap[HWnd];
    }
    
    void FocusViewportClient(HWND InAppWnd, uint32 InViewportClientIndex)
    {
        ActiveViewportWindow = InAppWnd;
        ActiveViewportClientIndex = InViewportClientIndex;
    }

    FEditorStateManager& GetEditorStateManager() { return EditorStateManager; }

/* Save And Load*/
private:
    const FString IniFilePath = "editor.ini";
public:
    void LoadConfig();
    void SaveConfig();
private:
    TMap<FString, FString> ReadIniFile(const FString& filePath);
    void WriteIniFile(const FString& filePath, const TMap<FString, FString>& config);

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
/* End of Save And Load */
};

