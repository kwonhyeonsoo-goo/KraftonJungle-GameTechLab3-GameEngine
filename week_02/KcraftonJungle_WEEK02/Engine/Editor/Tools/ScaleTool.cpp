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

bool ScaleTool::TryBeginManipulation(const MouseEvent& e, AppContext& ctx)
{
    if (!e.LeftDown)
        return false;

    ActiveAxis = EAxis::None;
    bDragging = false;
    DragStartAxisT = 0.0f;

    TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
    if (primaries.empty())
        return false;

    USceneComponent* primary = primaries.back();
    const FVector origin = primary->GetTransform().Location;

    const FMatrix viewProj = ctx.Editor.GetActiveViewport().Projection.Mode == EEditorProjectionMode::Orthographic
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

    OriginalTransforms.clear();
    for (auto& comp : primaries) {
        OriginalTransforms.push_back(comp->GetTransform());
    }

    return true;
}

void ScaleTool::OnMouseMove(const MouseEvent& e, AppContext& ctx)
{
    if (!bDragging)
    {
        TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
        if (primaries.empty()) HoveredAxis = EAxis::None; return;

        // 호버 감지
        USceneComponent* primary = primaries.back();

        const FVector origin = primary->GetTransform().Location;
        const FMatrix viewProj = ctx.Editor.GetActiveViewport().Projection.Mode == EEditorProjectionMode::Orthographic
            ? ctx.Editor.GetViewOrthoMatrix()
            : ctx.Editor.GetViewProjMatrix();
        const FVector2D mouse2D((float)e.X, (float)e.Y);
        const float gs = GizmoMath::ComputeGizmoScale(origin, ctx);

        float bestDistSq = GizmoMath::GizmoScaleBoxPx * GizmoMath::GizmoScaleBoxPx;
        EAxis bestAxis = EAxis::None;

        const ECoordSpace coordSpace = ECoordSpace::Local;
        for (int i = 0; i < 3; ++i)
        {
            const FVector axisDir = GizmoMath::GetAxisDirection(primary, i, coordSpace);
            const FVector handlePos = origin + axisDir * GizmoMath::GizmoAxisLength * gs;
            const FVector2D handle2D =
                GizmoMath::WorldToScreen(handlePos, viewProj, ctx.Window.GetWidth(), ctx.Window.GetHeight());

            if (!GizmoMath::PointInRect2D(mouse2D, handle2D, GizmoMath::GizmoScaleBoxPx))
                continue;

            const float dx = mouse2D.x - handle2D.x;
            const float dy = mouse2D.y - handle2D.y;
            const float distSq = dx * dx + dy * dy;
            if (distSq < bestDistSq) { bestDistSq = distSq; bestAxis = IndexToAxis(i); }
        }

        HoveredAxis = bestAxis;
        return;
    }

    TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
    if (primaries.empty()) {
        ActiveAxis = EAxis::None;
        bDragging = false;
        DragStartAxisT = 0.0f;
        return;
    }

    // 호버 감지
    USceneComponent* primary = primaries.back();

    const int axisIndex = AxisToIndex(ActiveAxis);
    if (axisIndex < 0)
        return;

    const FVector axisOrigin = OriginalTransforms.back().Location;
    const FVector axisDir = GizmoMath::GetAxisDirection(primary, axisIndex, ECoordSpace::Local);
    const Ray ray = GizmoMath::BuildMouseRay(e, ctx);

    float currentAxisT = 0.0f;
    if (!GizmoMath::ClosestAxisParameterToRay(axisOrigin, axisDir, ray, currentAxisT))
        return;

    const float delta = currentAxisT - DragStartAxisT;

    for (int32 i = 0; i < primaries.size(); i++) {
        Transform newTransform = OriginalTransforms[i];

        if (axisIndex == 0)
            newTransform.Scale.x = ClampScaleValue(OriginalTransforms[i].Scale.x + delta);
        else if (axisIndex == 1)
            newTransform.Scale.y = ClampScaleValue(OriginalTransforms[i].Scale.y + delta);
        else if (axisIndex == 2)
            newTransform.Scale.z = ClampScaleValue(OriginalTransforms[i].Scale.z + delta);

        primaries[i]->SetTransform(newTransform);
    }
}

void ScaleTool::OnMouseUp(const MouseEvent& e, AppContext& ctx)
{
    (void)e;

    if (!bDragging)
        return;

    TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
    if (primaries.empty()) return;

    for (int32 i = 0; i < primaries.size(); i++) {
        USceneComponent* primary = primaries[i];
        if (primary != nullptr)
        {
            const Transform finalTransform = primary->GetTransform();

            if (!NearlySameScale(finalTransform.Scale, OriginalTransforms[i].Scale))
            {
                primary->SetTransform(OriginalTransforms[i]);
                ctx.Dispatch(std::make_unique<SetTransformCommand>(primary, finalTransform));
            }
            else
            {
                primary->SetTransform(OriginalTransforms[i]);
            }
        }
    }
    OriginalTransforms.clear();

    ActiveAxis = EAxis::None;
    bDragging = false;
    DragStartAxisT = 0.0f;
}

void ScaleTool::FillGizmoState(AppContext& ctx, GizmoState& out) const
{
    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (!primary)
        return;

    const FVector origin = primary->GetTransform().Location;
    const float gs = GizmoMath::ComputeGizmoScale(origin, ctx);
    const float s = GizmoMath::GizmoAxisLength * gs;

    const FVector basisX = GizmoMath::GetAxisDirection(primary, 0, ECoordSpace::Local);
    const FVector basisY = GizmoMath::GetAxisDirection(primary, 1, ECoordSpace::Local);
    const FVector basisZ = GizmoMath::GetAxisDirection(primary, 2, ECoordSpace::Local);

    out.bActive = true;
    out.HoveredAxisIndex = bDragging ? -1 : AxisToIndex(HoveredAxis);
    out.ActiveAxisIndex = bDragging ? AxisToIndex(ActiveAxis) : -1;

    static const uint32 colors[3] = {
        GizmoMath::AxisColorX,
        GizmoMath::AxisColorY,
        GizmoMath::AxisColorZ
    };

    auto FillScaleAxis = [&](int axisIndex, const FVector& forwardY, const FVector& preferredX)
        {
            FVector localY = forwardY.Normalized();

            FVector localX = preferredX - localY * preferredX.Dot(localY);
            if (localX.IsNearlyZero())
                localX = GizmoMath::MakeStablePerpendicular(localY);
            else
                localX = localX.Normalized();

            const FVector localZ = localX.Cross(localY).Normalized();

            out.Axes[axisIndex].BaseColor = colors[axisIndex];
            out.Axes[axisIndex].WorldTransform = {
                localX.x * s, localX.y * s, localX.z * s, 0.0f,
                localY.x * s, localY.y * s, localY.z * s, 0.0f,
                localZ.x * s, localZ.y * s, localZ.z * s, 0.0f,
                origin.x,     origin.y,     origin.z,     1.0f
            };
            out.Axes[axisIndex].RenderType = ERenderType::ScaleGizmo;
        };

    FillScaleAxis(0, basisX, basisZ);
    FillScaleAxis(1, basisY, basisX);
    FillScaleAxis(2, basisZ, basisX);
}