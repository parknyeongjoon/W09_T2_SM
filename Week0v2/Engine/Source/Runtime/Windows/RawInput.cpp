// ReSharper disable CppClangTidyPerformanceNoIntToPtr
// ReSharper disable CppMemberFunctionMayBeConst

#include "RawInput.h"
#include <utility>
#include <stdexcept>

#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#define HID_USAGE_GENERIC_KEYBOARD     ((USHORT) 0x06)


FRawInput::FRawInput(InputCallback InCallback)
    : Callback(std::move(InCallback))
{
    RegisterDevices();
}

void FRawInput::ProcessRawInput(HWND AppWnd, LPARAM lParam) const
{
    uint32 DataSize = 0;

    // 입력 데이터 크기 확인
    GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &DataSize, sizeof(RAWINPUTHEADER));
    if (DataSize == 0) return;

    // 입력 데이터 버퍼 할당
    TArray<BYTE> Buffer;
    Buffer.SetNum(static_cast<int32>(DataSize));
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, Buffer.GetData(), &DataSize, sizeof(RAWINPUTHEADER)) == static_cast<uint32>(-1))
    {
        LogRawInput(LogLevel::Error, "Failed to get raw input data.");
        return;
    }

    // RAWINPUT 데이터 파싱
    const RAWINPUT* RawInput = reinterpret_cast<const RAWINPUT*>(Buffer.GetData());
    if (Callback)
    {
        Callback(AppWnd, *RawInput);
    }
}

bool FRawInput::IsValid() const
{
    return bIsValid;
}

void FRawInput::RegisterDevices()
{
    // 키보드 입력 장치 등록 (사용 페이지: 0x01, 사용: 0x06)
    const RAWINPUTDEVICE KeyboardRid = {
        HID_USAGE_PAGE_GENERIC,
        HID_USAGE_GENERIC_KEYBOARD,        // 키보드 사용
        // RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, // 백그라운드 입력 수신 및 장치 변경시 알림
        RIDEV_DEVNOTIFY,
        NULL                             // RawInput 메시지를 받을 대상 창 (NULL로 설정하여 등록 시점의 포커스 윈도우에서 처리)
    };

    if (!RegisterRawInputDevices(&KeyboardRid, 1, sizeof(KeyboardRid)))
    {
        LogRawInput(LogLevel::Error, "Keyboard registration failed: " + GetLastErrorString());
        return;
    }

    // 마우스 입력 장치 등록 (사용 페이지: 0x01, 사용: 0x02)
    const RAWINPUTDEVICE MouseRid = {
        HID_USAGE_PAGE_GENERIC,
        HID_USAGE_GENERIC_MOUSE,           // 마우스 사용
        // RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, // 백그라운드 입력 수신 및 장치 변경시 알림
        RIDEV_DEVNOTIFY,
        NULL                             // RawInput 메시지를 받을 대상 창 (NULL로 설정하여 등록 시점의 포커스 윈도우에서 처리)
    };

    if (!RegisterRawInputDevices(&MouseRid, 1, sizeof(MouseRid)))
    {
        LogRawInput(LogLevel::Error, "Mouse registration failed: " + GetLastErrorString());
        return;
    }

    bIsValid = true;
}

void FRawInput::UnregisterDevices()
{
    RAWINPUTDEVICE RawInputDevice = {};

    // 키보드 해제
    RawInputDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
    RawInputDevice.usUsage = HID_USAGE_GENERIC_KEYBOARD;
    RawInputDevice.dwFlags = RIDEV_REMOVE;
    RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RawInputDevice));

    // 마우스 해제
    RawInputDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
    RawInputDevice.usUsage = HID_USAGE_GENERIC_MOUSE;
    RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RawInputDevice));
}

std::string FRawInput::GetLastErrorString()
{
    const DWORD Error = GetLastError();
    if (Error == 0) return "";

    LPSTR Buffer = nullptr;
    const size_t Size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&Buffer), 0, nullptr
    );

    std::string Message(Buffer, Size);
    LocalFree(Buffer);
    return Message;
}

void FRawInput::LogRawInput(LogLevel Level, const std::string& Message)
{
    OutputDebugStringA(("[RawInput] " + Message + "\n").c_str());
    UE_LOG(Level, "[RawInput] %s", Message.c_str());
}
