#pragma once
#include "RawInput.h"
#include "Delegates/DelegateCombination.h"
#include "HAL/PlatformType.h"
#include "InputCore/InputCoreTypes.h"
#include "Math/Vector.h"
#include "SlateCore/Input/Events.h"

namespace EMouseButtons
{
enum Type : uint8;
}

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnKeyCharDelegate, const TCHAR /*Character*/, const bool /*IsRepeat*/, HWND /*AppWnd*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnKeyDownDelegate, const FKeyEvent& /*InKeyEvent*/, HWND /*AppWnd*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnKeyUpDelegate, const FKeyEvent& /*InKeyEvent*/, HWND /*AppWnd*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMouseDownDelegate, const FPointerEvent& /*InMouseEvent*/, HWND /*AppWnd*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMouseUpDelegate, const FPointerEvent& /*InMouseEvent*/, HWND /*AppWnd*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMouseDoubleClickDelegate, const FPointerEvent& /*InMouseEvent*/, HWND /*AppWnd*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMouseWheelDelegate, const FPointerEvent& /*InMouseEvent*/, HWND /*AppWnd*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMouseMoveDelegate, const FPointerEvent& /*InMouseEvent*/, HWND /*AppWnd*/);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRawMouseInputDelegate, const FPointerEvent& /*InRawMouseEvent*/, HWND /*AppWnd*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRawKeyboardInputDelegate, const FKeyEvent& /*InRawKeyboardEvent*/, HWND /*AppWnd*/);


class FSlateAppMessageHandler
{
public:
    FSlateAppMessageHandler();

    void ProcessMessage(HWND AppWnd, uint32 Msg, WPARAM wParam, LPARAM lParam);

public:
    /** Cursor와 관련된 변수를 업데이트 합니다. */
    void UpdateCursorPosition(const FVector2D& NewPos);

    /** 현재 마우스 포인터의 위치를 가져옵니다. */
    FVector2D GetCursorPos() const;

    /** 한 프레임 전의 마우스 포인터의 위치를 가져옵니다. */
    FVector2D GetLastCursorPos() const;

    /** ModifierKeys의 상태를 가져옵니다. */
    FModifierKeysState GetModifierKeys() const;

protected:
    void OnKeyChar(HWND AppWnd, TCHAR Character, bool IsRepeat);
    void OnKeyDown(HWND AppWnd, uint32 KeyCode, uint32 CharacterCode, bool IsRepeat);
    void OnKeyUp(HWND AppWnd, uint32 KeyCode, uint32 CharacterCode, bool IsRepeat);
    void OnMouseDown(HWND AppWnd, EMouseButtons::Type Button, FVector2D CursorPos);
    void OnMouseUp(HWND AppWnd, EMouseButtons::Type Button, FVector2D CursorPos);
    void OnMouseDoubleClick(HWND AppWnd, EMouseButtons::Type Button, FVector2D CursorPos);
    void OnMouseWheel(HWND AppWnd, float Delta, FVector2D CursorPos);
    void OnMouseMove(HWND AppWnd);

    void OnRawMouseInput(HWND AppWnd, const RAWMOUSE& RawMouseInput);
    void OnRawKeyboardInput(HWND AppWnd, const RAWKEYBOARD& RawKeyboardInput);
    // 추가적인 함수는 UnrealEngine [SlateApplication.h:1628]을 참조

public:
    FOnKeyCharDelegate OnKeyCharDelegate;
    FOnKeyDownDelegate OnKeyDownDelegate;
    FOnKeyUpDelegate OnKeyUpDelegate;

    FOnMouseDownDelegate OnMouseDownDelegate;
    FOnMouseUpDelegate OnMouseUpDelegate;
    FOnMouseDoubleClickDelegate OnMouseDoubleClickDelegate;
    FOnMouseWheelDelegate OnMouseWheelDelegate;
    FOnMouseMoveDelegate OnMouseMoveDelegate;

    FOnRawMouseInputDelegate OnRawMouseInputDelegate;
    FOnRawKeyboardInputDelegate OnRawKeyboardInputDelegate;

private:
    struct EModifierKey
    {
        enum Type : uint8
        {
            LeftShift,    // VK_LSHIFT
            RightShift,   // VK_RSHIFT
            LeftControl,  // VK_LCONTROL
            RightControl, // VK_RCONTROL
            LeftAlt,      // VK_LMENU
            RightAlt,     // VK_RMENU
            LeftWin,      // VK_LWIN
            RightWin,     // VK_RWIN
            CapsLock,     // VK_CAPITAL
            Count,
        };
    };

    // Cursor Position
    FVector2D CurrentPosition;
    FVector2D PreviousPosition;

    bool ModifierKeyState[EModifierKey::Count];
    TSet<EKeys::Type> PressedMouseButtons;

    std::shared_ptr<FRawInput> RawInputHandler;

private:
    void HandleRawInput(HWND AppWnd, const RAWINPUT& RawInput);
};
