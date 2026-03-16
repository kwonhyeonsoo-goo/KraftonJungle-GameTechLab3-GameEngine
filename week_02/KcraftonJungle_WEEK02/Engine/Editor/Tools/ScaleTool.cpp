#include "ScaleTool.h"

namespace
{
    constexpr float MinScaleValue = 0.01f;

    bool NearlySameScale(const FVector& a, const FVector& b, float eps = 0.0001f)
    {
        return std::fabs(a.x - b.x) <= eps
            && std::fabs(a.y - b.y) <= eps
            && std::fabs(a.z - b.z) <= eps;
    }

    float ClampScaleValue(float value)
    {
        return value < MinScaleValue ? MinScaleValue : value;
    }
}

int ScaleTool::AxisToIndex(EAxis axis)
{
    switch (axis)
    {
    case EAxis::X: return 0;
    case EAxis::Y: return 1;
    case EAxis::Z: return 2;
    default:       return -1;
    }
}

ScaleTool::EAxis ScaleTool::IndexToAxis(int index)
{
    switch (index)
    {
    case 0: return EAxis::X;
    case 1: return EAxis::Y;
    case 2: return EAxis::Z;
    default:return EAxis::None;
    }
}

bool ScaleTool::TryBeginManipulation(const MouseEvent& e, AppContext& ctx)
{
    if (!e.LeftDown)
        return false;

    ActiveAxis = EAxis::None;
    bDragging = false;
    DragStartAxisT = 0.0f;

    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (primary == nullptr)
        return false;

    const FVector origin = primary->GetTransform().Location;

    const FMatrix viewProj = ctx.Editor.bOrthoMode
        ? ctx.Editor.GetViewOrthoMatrix()
        : ctx.Editor.GetViewProjMatrix();
    const int32 viewportW = ctx.Window.GetWidth();
    const int32 viewportH = ctx.Window.GetHeight();
    const FVector2D mouse2D((float)e.X, (float)e.Y);

    float bestDistSq = (std::numeric_limits<float>::max)();
    EAxis bestAxis = EAxis::None;

    const float gizmoScale = GizmoMath::ComputeGizmoScale(origin, ctx);

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const FVector axisDir = GizmoMath::GetAxisDirection(primary, axisIndex, ECoordSpace::Local);
        const FVector handlePos = origin + axisDir * GizmoMath::GizmoAxisLength * gizmoScale;
        const FVector2D handle2D = GizmoMath::WorldToScreen(handlePos, viewProj, viewportW, viewportH);

        if (!GizmoMath::PointInRect2D(mouse2D, handle2D, GizmoMath::GizmoScaleBoxPx))
            continue;

        const float dx = mouse2D.x - handle2D.x;
        const float dy = mouse2D.y - handle2D.y;
        const float distSq = dx * dx + dy * dy;

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            bestAxis = IndexToAxis(axisIndex);
        }
    }

    if (bestAxis == EAxis::None)
        return false;

    const int axisIndex = AxisToIndex(bestAxis);
    const FVector axisDir = GizmoMath::GetAxisDirection(primary, axisIndex, ECoordSpace::Local);
    const Ray ray = GizmoMath::BuildMouseRay(e, ctx);

    float axisT = 0.0f;
    if (!GizmoMath::ClosestAxisParameterToRay(origin, axisDir, ray, axisT))
        return false;

    ActiveAxis = bestAxis;
    bDragging = true;
    DragStartAxisT = axisT;
    OriginalTransform = primary->GetTransform();

    return true;
}

void ScaleTool::OnMouseMove(const MouseEvent& e, AppContext& ctx)
{
    if (!bDragging)
        return;

    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (primary == nullptr)
    {
        ActiveAxis = EAxis::None;
        bDragging = false;
        DragStartAxisT = 0.0f;
        return;
    }

    const int axisIndex = AxisToIndex(ActiveAxis);
    if (axisIndex < 0)
        return;

    const FVector axisOrigin = OriginalTransform.Location;
    const FVector axisDir = GizmoMath::GetAxisDirection(primary, axisIndex, ECoordSpace::Local);
    const Ray ray = GizmoMath::BuildMouseRay(e, ctx);

    float currentAxisT = 0.0f;
    if (!GizmoMath::ClosestAxisParameterToRay(axisOrigin, axisDir, ray, currentAxisT))
        return;

    const float delta = currentAxisT - DragStartAxisT;

    Transform newTransform = OriginalTransform;

    if (axisIndex == 0)
        newTransform.Scale.x = ClampScaleValue(OriginalTransform.Scale.x + delta);
    else if (axisIndex == 1)
        newTransform.Scale.y = ClampScaleValue(OriginalTransform.Scale.y + delta);
    else if (axisIndex == 2)
        newTransform.Scale.z = ClampScaleValue(OriginalTransform.Scale.z + delta);

    primary->SetTransform(newTransform);
}

void ScaleTool::OnMouseUp(const MouseEvent& e, AppContext& ctx)
{
    (void)e;

    if (!bDragging)
        return;

    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (primary != nullptr)
    {
        const Transform finalTransform = primary->GetTransform();

        if (!NearlySameScale(finalTransform.Scale, OriginalTransform.Scale))
        {
            primary->SetTransform(OriginalTransform);
            ctx.Dispatch(std::make_unique<SetTransformCommand>(primary, finalTransform));
        }
        else
        {
            primary->SetTransform(OriginalTransform);
        }
    }

    ActiveAxis = EAxis::None;
    bDragging = false;
    DragStartAxisT = 0.0f;
}

void ScaleTool::BuildGizmoOverlay(AppContext& ctx, RenderQueue& queue)
{
    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (primary == nullptr)
        return;

    const FVector origin = primary->GetTransform().Location;
    const uint32 objectId = primary->GetUUID();

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const FVector axisDir = GizmoMath::GetAxisDirection(primary, axisIndex, ECoordSpace::Local);

        const uint32 color =
            axisIndex == 0 ? GizmoMath::AxisColorX :
            axisIndex == 1 ? GizmoMath::AxisColorY :
            GizmoMath::AxisColorZ;

        RenderCommand cmd = {};
        cmd.Type = ERenderType::Gizmo;
        const float gs = GizmoMath::ComputeGizmoScale(origin, ctx);
        cmd.WorldTransform = GizmoMath::MakeAxisTransform(origin, axisDir, GizmoMath::GizmoAxisLength * gs);
        cmd.bOrtho = ctx.Editor.bOrthoMode;
        cmd.Color = color;
        cmd.ObjectId = objectId;
        queue.Push(cmd);
    }
}
