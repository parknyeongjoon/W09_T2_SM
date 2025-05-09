#pragma once
#include "Container/Map.h"
#include "Core/HAL/PlatformType.h"


class ImGuiManager
{
public:

    static ImGuiManager& Get();
    
    void AddWindow(HWND hWnd, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
    void RemoveWindow(HWND hWnd);
    
    void BeginFrame(HWND hWnd);
    void EndFrame(HWND hWnd);
    void Release();
    bool GetWantCaptureMouse(HWND hWnd);
    bool GetWantCaptureKeyboard(HWND hWnd);
    struct ImGuiContext* GetImGuiContext(HWND hWnd);

private:
    void InitializeWindow();
    void PreferenceStyle();


private:
    TMap<HWND, ImGuiContext*> WindowContextMap;
};

