#pragma once
#include"../Foundation/Core/CoreTypes.h"

struct MouseMoveEvent { int32 X, Y, DeltaX, DeltaY; bool RightDown; };
struct MouseDownEvent { int32 X, Y; int32 Button; };
struct MouseUpEvent { int32 X, Y; int32 Button; };
struct KeyDownEvent { int32 KeyCode; };
struct KeyUpEvent { int32 KeyCode; };
struct ResizeEvent { int32 Width, Height; };

enum class EInputType { MouseMove, MouseDown, KeyDown /* ... */ };

struct FInputEvent {
    enum class EType {
        MouseMove, MouseDown, MouseUp,
        KeyDown, KeyUp, Resize
    };

    EType Type;

    union {
        MouseMoveEvent MouseMove;
        MouseDownEvent MouseDown;
        MouseUpEvent   MouseUp;
        KeyDownEvent   KeyDown;
        KeyUpEvent     KeyUp;
        ResizeEvent    Resize;
    };

    // 생성 편의 함수
    static FInputEvent Make(const MouseMoveEvent& e) {
        FInputEvent ev; ev.Type = EType::MouseMove; ev.MouseMove = e; return ev;
    }
    static FInputEvent Make(const MouseDownEvent& e) {
        FInputEvent ev; ev.Type = EType::MouseDown; ev.MouseDown = e; return ev;
    }
    static FInputEvent Make(const MouseUpEvent& e) {
        FInputEvent ev; ev.Type = EType::MouseUp;   ev.MouseUp = e; return ev;
    }
    static FInputEvent Make(const KeyDownEvent& e) {
        FInputEvent ev; ev.Type = EType::KeyDown;   ev.KeyDown = e; return ev;
    }
    static FInputEvent Make(const KeyUpEvent& e) {
        FInputEvent ev; ev.Type = EType::KeyUp;     ev.KeyUp = e; return ev;
    }
    static FInputEvent Make(const ResizeEvent& e) {
        FInputEvent ev; ev.Type = EType::Resize;    ev.Resize = e; return ev;
    }
};