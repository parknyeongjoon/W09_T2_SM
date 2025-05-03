#include "LaunchEngineLoop.h"

#include "ImGuiManager.h"
#include "Engine/Loader/FBX/FLoaderFBX.h"
#include "LevelEditor/SLevelEditor.h"
#include "PropertyEditor/ViewportTypePanel.h"
#include "Renderer/Renderer.h"
#include "UnrealEd/UnrealEd.h"
#include "UObject/Casts.h"


FGraphicsDevice FEngineLoop::GraphicDevice;
FRenderer FEngineLoop::Renderer;
FResourceManager FEngineLoop::ResourceManager;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

FEngineLoop::FEngineLoop()
    : AppWnd(nullptr)
    , ImGuiUIManager(nullptr)
{
}

int32 FEngineLoop::Init(HINSTANCE hInstance)
{
    WindowInit(hInstance);
    
    ImGuiUIManager = new ImGuiManager();
    AppMessageHandler = std::make_unique<FSlateAppMessageHandler>();
    
    GraphicDevice.Initialize(AppWnd);
    
    Renderer.Initialize(&GraphicDevice);
    ImGuiUIManager->Initialize(AppWnd, GraphicDevice.Device, GraphicDevice.DeviceContext);
    ResourceManager.Initialize(&GraphicDevice);

    
    // if (bIsEditor)
    {
        GEngine = FObjectFactory::ConstructObject<UEditorEngine>();
    }
    //else
    {
        //TODO : UENGINE으로 만들 수 있게 해주기 
        // GEngine = FObjectFactory::ConstructObject<UEngine>();
    }

    GEngine->Init(AppWnd);

    UpdateUI();

    // test
    FSkeletalMeshData data;
    FLoaderFBX loader = FLoaderFBX();
    if (loader.LoadSkeletalMesh("Contents/FBX/Wizard.fbx", data))
    {
        UE_LOG(LogLevel::Display, "load skeletal mesh succeed");
    }
    else
    {
        UE_LOG(LogLevel::Display, "load skeletal mesh succeed");
    }
    
    return 0;
}

void FEngineLoop::Tick()
{
    LARGE_INTEGER Frequency;
    const float targetFrameTime = 1000.0 / TargetFPS; // 한 프레임의 목표 시간 (밀리초 단위)

    QueryPerformanceFrequency(&Frequency);

    LARGE_INTEGER StartTime, EndTime;
    double ElapsedTime = 0.0;

    while (bIsExit == false)
    {
        QueryPerformanceCounter(&StartTime);

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg); // 키보드 입력 메시지를 문자메시지로 변경
            DispatchMessage(&msg);  // 메시지를 WndProc에 전달

            if (msg.message == WM_QUIT)
            {
                bIsExit = true;
                break;
            }
        }
        const float DeltaTime = static_cast<float>(ElapsedTime / 1000.f);
        
        ImGuiUIManager->BeginFrame();
        GEngine->Tick(DeltaTime);
        
        Render();
        
        Console::GetInstance().Draw();        
        ImGuiUIManager->EndFrame();

        // Pending 처리된 오브젝트 제거
        //GUObjectArray.ProcessPendingDestroyObjects();
        
        GraphicDevice.SwapBuffer();
        do
        {
            Sleep(0);
            QueryPerformanceCounter(&EndTime);
            ElapsedTime = static_cast<double>(EndTime.QuadPart - StartTime.QuadPart) * 1000.0 / static_cast<double>(Frequency.QuadPart);

        }
        while (ElapsedTime < targetFrameTime);

        UEngine::GFrameCount++;
    }
}

void FEngineLoop::Render()
{
    if (UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
    {
        if (SLevelEditor* LevelEditor = EditorEngine->GetLevelEditor())
        {
            GraphicDevice.Prepare();
            if (LevelEditor->IsMultiViewport())
            {
                const std::shared_ptr<FEditorViewportClient> OriginViewportClient = LevelEditor->GetActiveViewportClient();
                for (std::shared_ptr<FEditorViewportClient>& ViewportClient : LevelEditor->GetViewports())
                {
                    LevelEditor->SetViewportClient(ViewportClient);
                    EditorEngine->UpdateGizmos();    // TODO 임시 코드, EngineLoop로 옮기면서 그대로 냅둠
                    Renderer.Render(ViewportClient);
                }
                LevelEditor->SetViewportClient(OriginViewportClient);
            }
            else
            {
                EditorEngine->UpdateGizmos();    // TODO 임시 코드, EngineLoop로 옮기면서 그대로 냅둠
                Renderer.Render(LevelEditor->GetActiveViewportClient());
            }
        }
    }
}

void FEngineLoop::Exit()
{
    ImGuiUIManager->Shutdown();
    ResourceManager.Release();
    Renderer.Release();
    GraphicDevice.Release();
    
    GEngine->Release();

    delete ImGuiUIManager;
}

void FEngineLoop::ClearPendingCleanupObjects()
{
}

void FEngineLoop::WindowInit(HINSTANCE hInstance)
{
    WCHAR WindowClass[] = L"JungleWindowClass";

    WCHAR Title[] = L"GTL TARZAN SIU";

    WNDCLASSW wc{};
    wc.lpfnWndProc = AppWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WindowClass;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;

    RegisterClassW(&wc);

    AppWnd = CreateWindowExW(
        0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1400, 1000,
        nullptr, nullptr, hInstance, nullptr
    );
}

void FEngineLoop::UpdateUI() const
{
    Console::GetInstance().OnResize(AppWnd);
    if (UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
    {
        if (UnrealEd* UnrealEditor = EditorEngine->GetUnrealEditor())
        {
            UnrealEditor->OnResize(AppWnd);
        }
    }
}

LRESULT CALLBACK FEngineLoop::AppWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }
    
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            FEngineLoop::GraphicDevice.OnResize(hWnd);
            
            if (UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
            {
                if (SLevelEditor* LevelEditor = EditorEngine->GetLevelEditor())
                {
                    for (std::shared_ptr<FEditorViewportClient>& ViewportClient : LevelEditor->GetViewports())
                    {
                        ViewportClient->ResizeViewport(FEngineLoop::GraphicDevice.SwapchainDesc);
                    }
                }   
            }
        }
        ViewportTypePanel::GetInstance().OnResize(hWnd);
        GEngineLoop.UpdateUI();
        break;
    default:
        GEngineLoop.AppMessageHandler->ProcessMessage(hWnd, message, wParam, lParam);
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}
