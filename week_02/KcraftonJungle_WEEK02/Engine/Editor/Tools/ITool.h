#pragma once
#include "../../Foundation/Core/CoreTypes.h"
#include "../../Rendering/RenderQueue.h"

struct AppContext;

/// <summary>
/// int32 X, Y;
/// bool  LeftDown, RightDown, MiddleDown;
/// bool  Ctrl, Shift, Alt;
/// int32 DeltaX, DeltaY;
/// </summary>
struct MouseEvent {
    int32 X, Y;
    bool  LeftDown, RightDown, MiddleDown;
    bool  Ctrl, Shift, Alt;
    int32 DeltaX, DeltaY;
};

struct KeyEvent {
    int32 KeyCode;
    bool  IsDown;
};

class ITool {
public:
    virtual ~ITool() = default;
    virtual bool TryBeginManipulation(const MouseEvent& e, AppContext& ctx) { return false; }

    virtual void OnMouseMove(const MouseEvent& e, AppContext& ctx) {}
    virtual void OnMouseDown(const MouseEvent& e, AppContext& ctx) {}
    virtual void OnMouseUp(const MouseEvent& e, AppContext& ctx) {}
    virtual void OnKeyDown(const KeyEvent& e, AppContext& ctx) {}

    virtual void BuildGizmoOverlay(AppContext& ctx, RenderQueue& queue) {}

    virtual FString GetName() const = 0;
};