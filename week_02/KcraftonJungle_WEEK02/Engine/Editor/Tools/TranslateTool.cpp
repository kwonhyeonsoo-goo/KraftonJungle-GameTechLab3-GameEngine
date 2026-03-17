#include "TranslateTool.h"

bool TranslateTool::TryBeginManipulation(const MouseEvent& e, AppContext& ctx)
{
    if (!e.LeftDown)
        return false;

    ActiveAxis = EAxis::None;
    bDragging = false;
    DragStartAxisT = 0.0f;

    TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
    if(primaries.empty())
		return false;

	USceneComponent* primary = primaries.back();

    const FVector origin = primary->GetTransform().Location;
    const ECoordSpace coordSpace = ctx.Editor.Tools.GetCoordSpace();

    const FMatrix viewProj = ctx.Editor.GetActiveViewport().Projection.Mode == EEditorProjectionMode::Orthographic
        ? ctx.Editor.GetViewOrthoMatrix()
        : ctx.Editor.GetViewProjMatrix();
    const int32 viewportW = ctx.Window.GetWidth();
    const int32 viewportH = ctx.Window.GetHeight();
    const FVector2D mouse2D((float)e.X, (float)e.Y);

    const FVector2D screenOrigin =
        GizmoMath::WorldToScreen(origin, viewProj, viewportW, viewportH);

    float bestDistance = GizmoMath::GizmoPickThresholdPx;
    EAxis bestAxis = EAxis::None;

    const float gizmoScale = GizmoMath::ComputeGizmoScale(origin, ctx);

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const FVector axisDir = GizmoMath::GetAxisDirection(primary, axisIndex, coordSpace);
        const FVector tip = origin + axisDir * GizmoMath::GizmoAxisLength * gizmoScale;

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

    OriginalTransforms.clear();
    for(auto & comp: primaries) {
        OriginalTransforms.push_back(comp->GetTransform());
	}

    return true;
}

void TranslateTool::OnMouseMove(const MouseEvent& e, AppContext& ctx)
{
    if (bDragging)
    {
        TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
        if (primaries.empty()) {
            ActiveAxis = EAxis::None;
            bDragging = false;
            DragStartAxisT = 0.0f;
            return;
        }

        USceneComponent* primary = primaries.back();

        const int axisIndex = AxisToIndex(ActiveAxis);
        if (axisIndex < 0)
            return;

        const FVector axisOrigin = OriginalTransforms.back().Location;
        const FVector axisDir =
            GizmoMath::GetAxisDirection(primary, axisIndex, ctx.Editor.Tools.GetCoordSpace());

        const Ray ray = GizmoMath::BuildMouseRay(e, ctx);

        float currentAxisT = 0.0f;
        if (!GizmoMath::ClosestAxisParameterToRay(axisOrigin, axisDir, ray, currentAxisT))
            return;

        const float delta = currentAxisT - DragStartAxisT;

        FVector NewLocation = GizmoMath::GetSnapAppliedPosition(
            axisOrigin, axisDir, delta,
            ctx.Editor.Tools.IsSnapEnabled(),
            ctx.Editor.Tools.GetSnapValue());

        FVector DeltaLocation = NewLocation - axisOrigin;

        for (int32 i = 0; i < primaries.size(); i++) {
            Transform newTransform = OriginalTransforms[i];
			newTransform.Location += DeltaLocation;
            primaries[i]->SetTransform(newTransform);
        }
        return;
    }

    // 호버 감지
    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (primary == nullptr) { HoveredAxis = EAxis::None; return; }

    const FVector origin = primary->GetTransform().Location;
    const ECoordSpace coordSpace = ctx.Editor.Tools.GetCoordSpace();
    const FMatrix viewProj = ctx.Editor.GetActiveViewport().Projection.Mode == EEditorProjectionMode::Orthographic
        ? ctx.Editor.GetViewOrthoMatrix()
        : ctx.Editor.GetViewProjMatrix();
    const FVector2D mouse2D((float)e.X, (float)e.Y);
    const FVector2D screenOrigin =
        GizmoMath::WorldToScreen(origin, viewProj, ctx.Window.GetWidth(), ctx.Window.GetHeight());
    const float gs = GizmoMath::ComputeGizmoScale(origin, ctx);

    float bestDist = GizmoMath::GizmoPickThresholdPx;
    EAxis bestAxis = EAxis::None;

    for (int i = 0; i < 3; ++i)
    {
        const FVector axisDir = GizmoMath::GetAxisDirection(primary, i, coordSpace);
        const FVector tip = origin + axisDir * GizmoMath::GizmoAxisLength * gs;
        const FVector2D screenTip =
            GizmoMath::WorldToScreen(tip, viewProj, ctx.Window.GetWidth(), ctx.Window.GetHeight());
        const float dist = GizmoMath::DistancePointToSegment2D(mouse2D, screenOrigin, screenTip);
        if (dist < bestDist) { bestDist = dist; bestAxis = IndexToAxis(i); }
    }

    HoveredAxis = bestAxis;
}

void TranslateTool::OnMouseUp(const MouseEvent& e, AppContext& ctx)
{
    (void)e;

    if (!bDragging)
        return;

    TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
    if (primaries.empty()) return;

    for (int32 i = 0; i < primaries.size(); i++) {
		USceneComponent* primary = primaries[i];

        const Transform finalTransform = primary->GetTransform();

        if (!GizmoMath::NearlySameLocation(finalTransform.Location, OriginalTransforms[i].Location))
        {
            primary->SetTransform(OriginalTransforms[i]);
            ctx.Dispatch(std::make_unique<SetTransformCommand>(primary, finalTransform));
        }
        else
        {
            primary->SetTransform(OriginalTransforms[i]);
        }
    }
    OriginalTransforms.clear();

    ActiveAxis = EAxis::None;
    bDragging = false;
    DragStartAxisT = 0.0f;
}

void TranslateTool::FillAxisData(const FVector& origin, const FVector& axisDir,
                                  float scale, int /*axisIndex*/, GizmoAxisData& out) const
{
    out.WorldTransform = GizmoMath::MakeAxisTransform(
        origin, axisDir, GizmoMath::GizmoAxisLength * scale);
    out.RenderType = ERenderType::TranslateGizmo;
}
