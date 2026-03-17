#pragma once
#include "../../Foundation/Core/CoreTypes.h"
#include "../../Rendering/RenderTypes.h"

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

    virtual void FillGizmoState(AppContext& ctx, GizmoState& outState) const;

    virtual FString GetName() const = 0;

protected:
    virtual void FillAxisData(const FVector& origin, const FVector& axisDir,
                              float scale, int axisIndex, GizmoAxisData& out) const {}

    enum class EAxis { None, X, Y, Z };

    static int   AxisToIndex(EAxis axis);
    static EAxis IndexToAxis(int index);

    EAxis ActiveAxis  = EAxis::None;
    EAxis HoveredAxis = EAxis::None;
    bool  bDragging   = false;
};