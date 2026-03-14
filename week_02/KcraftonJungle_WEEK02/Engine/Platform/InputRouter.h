// InputRouter.h
#pragma once
#include <windows.h>
#include "InputEvent.h"
#include "../Foundation/Core/CoreTypes.h"

struct InputState {
    int32 MouseX = 0, MouseY = 0;
    int32 MouseDeltaX = 0, MouseDeltaY = 0;
    bool  MouseButtons[3] = {};
    bool  Keys[256] = {};

    bool IsMouseCapturedByImGui()    const;
    bool IsKeyboardCapturedByImGui() const;
    // ★ 선언만 ? 구현은 .cpp로
};

class InputRouter {
public:
    static LRESULT             Route(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static const InputState& GetState();
    static TArray<FInputEvent> ConsumePendingEvents();
    static void                ResetDelta();

private:
    static InputState          State;
    static int32               LastMouseX, LastMouseY;
    static TArray<FInputEvent> Events;
};