#pragma once
#include <functional>
#include <string>

#include "UserInterface/Console.h"


class FRawInput
{
public:
    using InputCallback = std::function<void(HWND AppWnd, const RAWINPUT& RawInput)>;

    FRawInput(InputCallback InCallback);

    ~FRawInput()
    {
        UnregisterDevices();
    }

    FRawInput(const FRawInput&) = delete;
    FRawInput& operator=(const FRawInput&) = delete;
    FRawInput(FRawInput&&) = delete;
    FRawInput& operator=(FRawInput&&) = delete;

public:
    // 메시지 처리 함수
    void ProcessRawInput(HWND AppWnd, LPARAM lParam) const;

    void ReRegisterDevices()
    {
        UnregisterDevices();
        RegisterDevices();
    }

    bool IsValid() const;

private:
    void RegisterDevices();
    void UnregisterDevices();

    static std::string GetLastErrorString();
    static void LogRawInput(LogLevel Level, const std::string& Message);

private:
    InputCallback Callback;

    bool bIsValid = false;
};
