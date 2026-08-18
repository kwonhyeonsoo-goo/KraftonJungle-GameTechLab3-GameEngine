#include "InputRouter.h"
#include "../../Editor/ImGui/imgui.h" // ImGui는 cpp에서만 include
#include <windowsx.h>

// static 멤버 정의
InputState          InputRouter::State;
int32               InputRouter::LastMouseX = 0;
int32               InputRouter::LastMouseY = 0;
TArray<FInputEvent> InputRouter::Events;
bool InputRouter::bMouseDeltaValid = false;
bool InputRouter::bTrackingMouseLeave = false;

bool InputState::IsMouseCapturedByImGui() const {
    return ImGui::GetIO().WantCaptureMouse;
}

bool InputState::IsKeyboardCapturedByImGui() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

LRESULT InputRouter::Route(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_MOUSEMOVE:{
        if (!bTrackingMouseLeave)
        {
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            bTrackingMouseLeave = true;
        }

        int32 xPos = GET_X_LPARAM(lp); // X 좌표 꺼내기
        int32 yPos = GET_Y_LPARAM(lp); // Y 좌표 꺼내기

        if (!bMouseDeltaValid)
        {
            State.MouseDeltaX = 0;
            State.MouseDeltaY = 0;
            bMouseDeltaValid = true;
        }
        else
        {
            State.MouseDeltaX = xPos - LastMouseX;
            State.MouseDeltaY = yPos - LastMouseY;
        }
        State.MouseX = xPos;
        State.MouseY = yPos;
        LastMouseX = xPos;
        LastMouseY = yPos;

        // ② 큐에 적재 (브로드캐스트는 Poll()이 함)
        Events.push_back(FInputEvent::Make(
            MouseMoveEvent{ xPos, yPos, State.MouseDeltaX, State.MouseDeltaY,
                            State.MouseButtons[1] }));
        break;
    }
    case WM_MOUSELEAVE:
        bTrackingMouseLeave = false;
        bMouseDeltaValid = false;
        State.MouseDeltaX = 0;
        State.MouseDeltaY = 0;
        break;

    case WM_LBUTTONDOWN:
        State.MouseButtons[0] = true;
        Events.push_back(FInputEvent::Make(MouseDownEvent{ LOWORD(lp), HIWORD(lp), 0 }));
        break;

    case WM_NCMOUSEMOVE:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONUP:
    case WM_CAPTURECHANGED:
        bMouseDeltaValid = false;
        State.MouseDeltaX = 0;
        State.MouseDeltaY = 0;
        break;

    case WM_RBUTTONDOWN:
        State.MouseButtons[1] = true;
        Events.push_back(FInputEvent::Make(MouseDownEvent{ LOWORD(lp), HIWORD(lp), 1 }));
        break;

    case WM_LBUTTONUP:
        State.MouseButtons[0] = false;
        Events.push_back(FInputEvent::Make(MouseUpEvent{ LOWORD(lp), HIWORD(lp), 0 }));
        break;

    case WM_RBUTTONUP:
        State.MouseButtons[1] = false;
        Events.push_back(FInputEvent::Make(MouseUpEvent{ LOWORD(lp), HIWORD(lp), 1 }));
        break;

    case WM_KEYDOWN:
        State.Keys[wp] = true;
        Events.push_back(FInputEvent::Make(KeyDownEvent{ (int32)wp }));
        break;

    case WM_KEYUP:
        State.Keys[wp] = false;
        Events.push_back(FInputEvent::Make(KeyUpEvent{ (int32)wp }));
        break;

    case WM_SIZE:
        Events.push_back(FInputEvent::Make(ResizeEvent{ LOWORD(lp), HIWORD(lp) }));
        break;
    }
    return 0;
}

const InputState& InputRouter::GetState()
{
    // TODO: 여기에 return 문을 삽입합니다.
    return State;
}

TArray<FInputEvent> InputRouter::ConsumePendingEvents()
{
    TArray<FInputEvent> out = Events; // 복사
    Events.clear();                   // 원본 비우기
    return out;
}

void InputRouter::ResetDelta()
{
    State.MouseDeltaX = 0;
    State.MouseDeltaY = 0;
}

void InputRouter::SetWindowFocus(bool bIsFocus)
{
    if (!bIsFocus)
    {
        for (auto& Key : State.Keys)
        {
            Key = false;
        }

        for (int i = 0; i < 3; ++i)
        {
            State.MouseButtons[i] = false;
        }

        State.MouseDeltaX = 0;
        State.MouseDeltaY = 0;
        bMouseDeltaValid = false;
        bTrackingMouseLeave = false;
    }
}
