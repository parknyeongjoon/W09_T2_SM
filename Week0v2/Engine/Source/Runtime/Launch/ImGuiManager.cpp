#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_dx11.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGuiManager.h"
#include "Font/RawFonts.h"
#include "Font/IconDefs.h"

ImGuiManager& ImGuiManager::Get()
{
    static ImGuiManager Instance;
    return Instance;
}

void ImGuiManager::AddWindow(HWND hWnd, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
{
    if (WindowContextMap.Contains(hWnd))
    {
        return;
    }

    ImGuiContext* OriginalContext = ImGui::GetCurrentContext();

    // ImGui Context 생성
    ImGuiContext* Context = ImGui::CreateContext();
    ImGui::SetCurrentContext(Context);

    // Win32 백엔드 초기화 (핸들 마다)
    ImGui_ImplWin32_Init(hWnd);
    // DX11 백엔드 초기화 (전역에 한 번만)
    ImGuiIO& io = ImGui::GetIO();
    if (io.BackendRendererUserData == nullptr)
    {
        ImGui_ImplDX11_Init(Device, DeviceContext);
    }

    WindowContextMap.Add(hWnd, Context);

    InitializeWindow();

    ImGui::SetCurrentContext(OriginalContext);
}

void ImGuiManager::RemoveWindow(HWND hWnd)
{
    if (!WindowContextMap.Contains(hWnd))
    {
        return;
    }

    ImGui::SetCurrentContext(WindowContextMap[hWnd]);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext(WindowContextMap[hWnd]);

    WindowContextMap.Remove(hWnd);
}

void ImGuiManager::InitializeWindow()
{
    IMGUI_CHECKVERSION();
    ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // ImGuiStyle& style = ImGui::GetStyle();
    // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    // {
    //     style.WindowRounding = 0.0f;
    //     style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    // }
    
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\malgun.ttf", 17.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    ImGui::GetStyle().ScaleAllSizes(0.8f);

    ImFontConfig FeatherFontConfig;
    FeatherFontConfig.PixelSnapH = true;
    FeatherFontConfig.FontDataOwnedByAtlas = false;
    FeatherFontConfig.GlyphOffset = ImVec2(0, 0);
    static const ImWchar IconRanges[] = {
        ICON_MOVE,      ICON_MOVE + 1,
        ICON_ROTATE,    ICON_ROTATE + 1,
        ICON_SCALE,     ICON_SCALE + 1,
        ICON_MONITOR,   ICON_MONITOR + 1,
        ICON_BAR_GRAPH, ICON_BAR_GRAPH + 1,
        ICON_NEW,       ICON_NEW + 1,
        ICON_SAVE,      ICON_SAVE + 1,
        ICON_LOAD,      ICON_LOAD + 1,
        ICON_MENU,      ICON_MENU + 1,
        ICON_SLIDER,    ICON_SLIDER + 1,
        ICON_PLUS,      ICON_PLUS + 1,
        ICON_PIE_PLAY, ICON_PIE_PLAY + 1,
        ICON_PIE_PAUSE, ICON_PIE_PAUSE + 1,
        ICON_PIE_STOP, ICON_PIE_STOP + 1,
        0 };

    io.Fonts->AddFontFromMemoryTTF(FeatherRawData, FontSizeOfFeather, 22.0f, &FeatherFontConfig, IconRanges);
    PreferenceStyle();
}

void ImGuiManager::BeginFrame(HWND hWnd)
{
    if (!WindowContextMap.Contains(hWnd))
    {
        return;
    }

    ImGui::SetCurrentContext(WindowContextMap[hWnd]);
    
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndFrame(HWND hWnd)
{
    if (!WindowContextMap.Contains(hWnd))
    {
        return;
    }
    
    ImGui::Render();
    
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

/* GUI Style Preference */
void ImGuiManager::PreferenceStyle()
{
    // Window
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.9f);
    // Title
    ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImVec4{ 0.02f, 0.02f, 0.02f, 1.0f };
    ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.02f, 0.02f, 0.02f, 1.0f };
    ImGui::GetStyle().WindowRounding = 5.0f;
    ImGui::GetStyle().PopupRounding = 3.0f;
    ImGui::GetStyle().FrameRounding = 3.0f;

    // Sep
    ImGui::GetStyle().Colors[ImGuiCol_Separator] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

    // Popup
    ImGui::GetStyle().Colors[ImGuiCol_PopupBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.9f);
    
    // Frame
    ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    ImGui::GetStyle().Colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Button
    ImGui::GetStyle().Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0, 0.0f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(0.105f, 0.105f, 0.105f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.00f, 0.85f, 1.0f);

    // Header
    ImGui::GetStyle().Colors[ImGuiCol_Header] = ImVec4(0.203f, 0.203f, 0.203f, 0.6f);
    ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] = ImVec4(0.105f, 0.105f, 0.105f, 0.6f);
    ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.85f, 0.85f);

    // Text
    ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 0.9f);

    // Tabs
    ImGui::GetStyle().Colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    ImGui::GetStyle().Colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
    ImGui::GetStyle().Colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
    ImGui::GetStyle().Colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    ImGui::GetStyle().Colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
}

void ImGuiManager::Release()
{
    TMap<HWND, ImGuiContext*> CopiedMap = WindowContextMap;
    for (auto& [HWnd, WindowContext] : CopiedMap)
    {
        RemoveWindow(HWnd);
    }

    WindowContextMap.Empty();
}

bool ImGuiManager::GetWantCaptureMouse(HWND hWnd)
{
    if (!WindowContextMap.Contains(hWnd))
    {
       return false;
    }

    bool bWantCaptureMouse;

    ImGuiContext* OriginalContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(WindowContextMap[hWnd]);
    {
        bWantCaptureMouse = ImGui::GetIO().WantCaptureMouse;
    }
    ImGui::SetCurrentContext(OriginalContext);

    return bWantCaptureMouse;
}

bool ImGuiManager::GetWantCaptureKeyboard(HWND hWnd)
{
    if (!WindowContextMap.Contains(hWnd))
    {
        return false;
    }

    bool bWantCaptureKeyboard;

    ImGuiContext* OriginalContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(WindowContextMap[hWnd]);
    {
        bWantCaptureKeyboard = ImGui::GetIO().WantCaptureKeyboard;
    }
    ImGui::SetCurrentContext(OriginalContext);

    return bWantCaptureKeyboard;
}

ImGuiContext* ImGuiManager::GetImGuiContext(HWND hWnd)
{
    if (!WindowContextMap.Contains(hWnd))
    {
        return nullptr;
    }

    return WindowContextMap[hWnd];
}

