#pragma once
#include "SLevelEditor.h"
#include "SlateCore/Widgets/SWindow.h"
#include "Slate/Widgets/Layout/SSplitter.h"
#include "UnrealClient.h"
#include "UnrealEd/EditorViewportClient.h"
#include "EditorEngine.h"
#include "fstream"
#include "sstream"
#include "ostream"
extern UEditorEngine* GEngine;

SLevelEditor::SLevelEditor() : bInitialize(false), HSplitter(nullptr), VSplitter(nullptr),
World(nullptr), bMultiViewportMode(false)
{
}

SLevelEditor::~SLevelEditor()
{
}

void SLevelEditor::Initialize()
{
    for (size_t i = 0; i < 4; i++)
    {
        viewportClients[i] = std::make_shared<FEditorViewportClient>();
        viewportClients[i]->Initialize(i);
    }

    OnResize();
    VSplitter = new SSplitterV();
    VSplitter->Initialize(FRect(0.0f, EditorHeight * 0.5f - 10, EditorHeight, 20));
    VSplitter->OnDrag(FPoint(0, 0));
    HSplitter = new SSplitterH();
    HSplitter->Initialize(FRect(EditorWidth * 0.5f - 10, 0.0f, 20, EditorWidth));
    HSplitter->OnDrag(FPoint(0, 0));
    LoadConfig();
    bInitialize = true;
}

void SLevelEditor::Tick(ELevelTick tickType, double deltaTime)
{
    if (tickType == LEVELTICK_ViewportsOnly)
    {
        if (bMultiViewportMode) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(GEngine->hWnd, &pt);
            if (VSplitter->IsHover(FPoint(pt.x, pt.y)) || HSplitter->IsHover(FPoint(pt.x, pt.y)))
            {
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
            }
            else
            {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            Input();
        }
    }
    //Test Code Cursor icon End
    OnResize();
    ActiveViewportClient->Tick(deltaTime);

}

void SLevelEditor::Input()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
    {
        if (bLButtonDown == false)
        {
            bLButtonDown = true;
            POINT pt;
            GetCursorPos(&pt);
            GetCursorPos(&lastMousePos);
            ScreenToClient(GEngine->hWnd, &pt);

            SelectViewport(pt);

            VSplitter->OnPressed(FPoint(pt.x, pt.y));
            HSplitter->OnPressed(FPoint(pt.x, pt.y));
        }
        else
        {
            POINT currentMousePos;
            GetCursorPos(&currentMousePos);

            // 마우스 이동 차이 계산
            int32 deltaX = currentMousePos.x - lastMousePos.x;
            int32 deltaY = currentMousePos.y - lastMousePos.y;

            if (VSplitter->IsPressing())
            {
                VSplitter->OnDrag(FPoint(deltaX, deltaY));
            }
            if (HSplitter->IsPressing())
            {
                HSplitter->OnDrag(FPoint(deltaX, deltaY));
            }
            ResizeViewports();
            lastMousePos = currentMousePos;
        }
    }
    else
    {
        bLButtonDown = false;
        VSplitter->OnReleased();
        HSplitter->OnReleased();
    }
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        if (!bRButtonDown)
        {
            bRButtonDown = true;
            POINT pt;
            GetCursorPos(&pt);
            GetCursorPos(&lastMousePos);
            ScreenToClient(GEngine->hWnd, &pt);

            SelectViewport(pt);
        }
    }
    else
    {
        bRButtonDown = false;
    }
}

void SLevelEditor::Release()
{
    SaveConfig();
    delete VSplitter;
    delete HSplitter;
}

void SLevelEditor::SelectViewport(POINT point)
{
    for (int i = 0; i < 4; i++)
    {
        if (viewportClients[i]->IsSelected(point))
        {
            SetViewportClient(i);
            break;
        }
    }
}

void SLevelEditor::OnResize()
{
    float PrevWidth = EditorWidth;
    float PrevHeight = EditorHeight;
    EditorWidth = GEngine->graphicDevice.screenWidth;
    EditorHeight = GEngine->graphicDevice.screenHeight;
    if (bInitialize) {
        //HSplitter 에는 바뀐 width 비율이 들어감 
        HSplitter->OnResize(EditorWidth / PrevWidth, EditorHeight);
        //HSplitter 에는 바뀐 Height 비율이 들어감 
        VSplitter->OnResize(EditorWidth, EditorHeight / PrevHeight);
        ResizeViewports();
    }
}

void SLevelEditor::ResizeViewports()
{
    if (bMultiViewportMode) {
        if (GetViewports()[0]) {
            for (int i = 0; i < 4; ++i)
            {
                GetViewports()[i]->ResizeViewport(VSplitter->SideLT->Rect, VSplitter->SideRB->Rect,
                    HSplitter->SideLT->Rect, HSplitter->SideRB->Rect);
            }
        }
    }
    else
    {
        ActiveViewportClient->GetViewport()->ResizeViewport(FRect(0.0f, 0.0f, EditorWidth, EditorHeight));
    }
}

void SLevelEditor::OnMultiViewport()
{
    bMultiViewportMode = true;
    ResizeViewports();
}

void SLevelEditor::OffMultiViewport()
{
    bMultiViewportMode = false;
}

bool SLevelEditor::IsMultiViewport()
{
    return bMultiViewportMode;
}

void SLevelEditor::LoadConfig()
{
    auto config = ReadIniFile(IniFilePath);
    ActiveViewportClient->Pivot.X = GetValueFromConfig(config, "OrthoPivotX", 0.0f);
    ActiveViewportClient->Pivot.Y = GetValueFromConfig(config, "OrthoPivotY", 0.0f);
    ActiveViewportClient->Pivot.Z = GetValueFromConfig(config, "OrthoPivotZ", 0.0f);
    ActiveViewportClient->orthoSize = GetValueFromConfig(config, "OrthoZoomSize", 10.0f);

    SetViewportClient(GetValueFromConfig(config, "ActiveViewportIndex", 0));
    //bMultiViewportMode = GetValueFromConfig(config, "bMutiView", false);
    bMultiViewportMode = false;
    for (size_t i = 0; i < 4; i++)
    {
        viewportClients[i]->LoadConfig(config);
    }
    if (HSplitter)
        HSplitter->LoadConfig(config);
    if (VSplitter)
        VSplitter->LoadConfig(config);

}

void SLevelEditor::SaveConfig()
{
    TMap<FString, FString> config;
    if (HSplitter)
        HSplitter->SaveConfig(config);
    if (VSplitter)
        VSplitter->SaveConfig(config);
    for (size_t i = 0; i < 4; i++)
    {
        viewportClients[i]->SaveConfig(config);
    }
    ActiveViewportClient->SaveConfig(config);
    config["bMutiView"] = std::to_string(bMultiViewportMode);
    config["ActiveViewportIndex"] = std::to_string(ActiveViewportClient->ViewportIndex);
    config["ScreenWidth"] = std::to_string(ActiveViewportClient->ViewportIndex);
    config["ScreenHeight"] = std::to_string(ActiveViewportClient->ViewportIndex);
    config["OrthoPivotX"] = std::to_string(ActiveViewportClient->Pivot.X);
    config["OrthoPivotY"] = std::to_string(ActiveViewportClient->Pivot.Y);
    config["OrthoPivotZ"] = std::to_string(ActiveViewportClient->Pivot.Z);
    config["OrthoZoomSize"] = std::to_string(ActiveViewportClient->orthoSize);
    WriteIniFile(IniFilePath, config);
}

TMap<FString, FString> SLevelEditor::ReadIniFile(const FString& filePath)
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

void SLevelEditor::WriteIniFile(const FString& filePath, const TMap<FString, FString>& config)
{
    std::ofstream file(*filePath);
    for (const auto& pair : config) {
        file << *pair.Key << "=" << *pair.Value << "\n";
    }
}

