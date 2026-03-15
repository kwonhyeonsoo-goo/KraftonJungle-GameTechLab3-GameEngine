#include "TranslateTool.h"

namespace
{
    void PushLine(RenderQueue& queue,
        const FVector& a,
        const FVector& b,
        uint32 color,
        uint32 objectId)
    {
        RenderCommand cmd = {};
        cmd.Type = ERenderType::Gizmo;
        cmd.Color = color;
        cmd.ObjectId = objectId;
        queue.Push(cmd);
    }
}

int TranslateTool::AxisToIndex(EAxis axis)
{
    switch (axis)
    {
    case EAxis::X: return 0;
    case EAxis::Y: return 1;
    case EAxis::Z: return 2;
    default:       return -1;
    }
}

TranslateTool::EAxis TranslateTool::IndexToAxis(int index)
{
    switch (index)
    {
    case 0: return EAxis::X;
    case 1: return EAxis::Y;
    case 2: return EAxis::Z;
    default:return EAxis::None;
    }
}

bool TranslateTool::TryBeginManipulation(const MouseEvent& e, AppContext& ctx)
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
    const ECoordSpace coordSpace = ctx.Editor.Tools.GetCoordSpace();

    const FMatrix viewProj = ctx.Editor.GetViewProjMatrix();
    const int32 viewportW = ctx.Window.GetWidth();
    const int32 viewportH = ctx.Window.GetHeight();
    const FVector2D mouse2D((float)e.X, (float)e.Y);

    const FVector2D screenOrigin =
        GizmoMath::WorldToScreen(origin, viewProj, viewportW, viewportH);

    float bestDistance = GizmoMath::GizmoPickThresholdPx;
    EAxis bestAxis = EAxis::None;

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const FVector axisDir = GizmoMath::GetAxisDirection(primary, axisIndex, coordSpace);
        const FVector tip = origin + axisDir * GizmoMath::GizmoAxisLength;

        const FVector2D screenTip =
            GizmoMath::WorldToScreen(tip, viewProj, viewportW, viewportH);

        const float dist =
            GizmoMath::DistancePointToSegment2D(mouse2D, screenOrigin, screenTip);

        if (dist < bestDistance)
        {
            bestDistance = dist;
            bestAxis = IndexToAxis(axisIndex);
        }
    }

    if (bestAxis == EAxis::None)
        return false;

    const int axisIndex = AxisToIndex(bestAxis);
    const FVector axisDir = GizmoMath::GetAxisDirection(primary, axisIndex, coordSpace);
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

void TranslateTool::OnMouseMove(const MouseEvent& e, AppContext& ctx)
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
    const FVector axisDir =
        GizmoMath::GetAxisDirection(primary, axisIndex, ctx.Editor.Tools.GetCoordSpace());

    const Ray ray = GizmoMath::BuildMouseRay(e, ctx);

    float currentAxisT = 0.0f;
    if (!GizmoMath::ClosestAxisParameterToRay(axisOrigin, axisDir, ray, currentAxisT))
        return;

    const float delta = currentAxisT - DragStartAxisT;

    Transform newTransform = OriginalTransform;
    newTransform.Location = GizmoMath::GetSnapAppliedPosition(
        axisOrigin,
        axisDir,
        delta,
        ctx.Editor.Tools.IsSnapEnabled(),
        ctx.Editor.Tools.GetSnapValue());

    primary->SetTransform(newTransform);
}

void TranslateTool::OnMouseUp(const MouseEvent& e, AppContext& ctx)
{
    (void)e;

    if (!bDragging)
        return;

    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (primary != nullptr)
    {
        const Transform finalTransform = primary->GetTransform();

        if (!GizmoMath::NearlySameLocation(finalTransform.Location, OriginalTransform.Location))
        {
            primary->SetTransform(OriginalTransform);
            ctx.Dispatch(new SetTransformCommand(primary, finalTransform));
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

static FMatrix MakeArrowTransformFromLocalZ(
    const FVector& basePos,
    const FVector& axisDir,
    float uniformScale = 1.0f)
{
    FVector localZ = axisDir.Normalized();
    FVector localX = GizmoMath::MakeStablePerpendicular(localZ);
    FVector localY = localZ.Cross(localX).Normalized();

    // row-vector ±âÁØ:
    // row0 = local X axis in world
    // row1 = local Y axis in world
    // row2 = local Z axis in world
    // row3 = translation
    return {
        localX.x * uniformScale, localX.y * uniformScale, localX.z * uniformScale, 0.0f,
        localY.x * uniformScale, localY.y * uniformScale, localY.z * uniformScale, 0.0f,
        localZ.x * uniformScale, localZ.y * uniformScale, localZ.z * uniformScale, 0.0f,
        basePos.x,               basePos.y,               basePos.z,               1.0f
    };
}

void TranslateTool::BuildGizmoOverlay(AppContext& ctx, RenderQueue& queue)
{
    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (primary == nullptr)
        return;

    const FVector origin = primary->GetTransform().Location;
    const ECoordSpace coordSpace = ctx.Editor.Tools.GetCoordSpace();

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const FVector axisDir = GizmoMath::GetAxisDirection(primary, axisIndex, coordSpace);

        const uint32 color =
            axisIndex == 0 ? GizmoMath::AxisColorX :
            axisIndex == 1 ? GizmoMath::AxisColorY :
            GizmoMath::AxisColorZ;

        RenderCommand cmd = {};
        cmd.Type = ERenderType::Gizmo;
        cmd.WorldTransform = MakeArrowTransformFromLocalZ(origin, axisDir, 1.0f);
        cmd.Color = color;
        cmd.ObjectId = primary->GetUUID();

        queue.Push(cmd);
    }
}