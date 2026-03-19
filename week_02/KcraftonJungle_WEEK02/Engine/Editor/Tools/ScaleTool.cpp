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

    bool TryComputeAxisParameterFromMouse(
        const MouseEvent& e,
        AppContext& ctx,
        const FVector& axisOrigin,
        const FVector& axisDir,
        float& outAxisT)
    {
        const Ray ray = GizmoMath::BuildMouseRay(e, ctx);
        return GizmoMath::ClosestAxisParameterToRay(axisOrigin, axisDir, ray, outAxisT);
    }

    Transform BuildScaledTransform(
        const Transform& original,
        int activeAxisIndex,
        bool bUniformScale,
        float delta)
    {
        Transform result = original;

        auto ApplyAxis = [&](int axisIndex, float& value)
            {
                if (bUniformScale || axisIndex == activeAxisIndex)
                {
                    value = ClampScaleValue(value + delta);
                }
            };

        ApplyAxis(0, result.Scale.x);
        ApplyAxis(1, result.Scale.y);
        ApplyAxis(2, result.Scale.z);

        return result;
    }
}

bool ScaleTool::TryPickAxisHandle(const MouseEvent& e, AppContext& ctx, EAxis& outAxis) const
{
    outAxis = EAxis::None;

    TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
    if (primaries.empty())
        return false;

    USceneComponent* primary = primaries.back();
    if (primary == nullptr)
        return false;

    const FVector origin = primary->GetTransform().Location;
    const FMatrix viewProj = ctx.Editor.GetActiveViewport().Projection.Mode == EEditorProjectionMode::Orthographic
        ? ctx.Editor.GetViewOrthoMatrix()
        : ctx.Editor.GetViewProjMatrix();

    const int32 viewportW = ctx.Window.GetWidth();
    const int32 viewportH = ctx.Window.GetHeight();
    const FVector2D mouse2D((float)e.X, (float)e.Y);
    const float gizmoScale = GizmoMath::ComputeGizmoScale(origin, ctx);

    float bestDistSq = (std::numeric_limits<float>::max)();
    EAxis bestAxis = EAxis::None;

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

    outAxis = bestAxis;
    return true;
}

void ScaleTool::ApplyScaleDelta(const TArray<USceneComponent*>& primaries, float delta) const
{
    const int activeAxisIndex = AxisToIndex(ActiveAxis);
    if (activeAxisIndex < 0)
        return;

    const int32 count = static_cast<int32>(primaries.size());
    for (int32 i = 0; i < count; ++i)
    {
        USceneComponent* component = primaries[i];
        if (component == nullptr)
            continue;

        const Transform newTransform = BuildScaledTransform(
            OriginalTransforms[i],
            activeAxisIndex,
            bActiveUniformScale,
            delta);

        component->SetTransform(newTransform);
    }
}

void ScaleTool::ResetManipulationState()
{
    ActiveAxis = EAxis::None;
    bDragging = false;
    DragStartAxisT = 0.0f;
    bActiveUniformScale = false;
    DragAxisOrigin = FVector{};
    DragAxisDirection = FVector{};
}

bool ScaleTool::TryBeginManipulation(const MouseEvent& e, AppContext& ctx)
{
    if (!e.LeftDown)
        return false;

    ResetManipulationState();
    OriginalTransforms.clear();

    TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
    if (primaries.empty())
        return false;

    USceneComponent* primary = primaries.back();
    if (primary == nullptr)
        return false;

    EAxis pickedAxis = EAxis::None;
    if (!TryPickAxisHandle(e, ctx, pickedAxis))
        return false;

    const int axisIndex = AxisToIndex(pickedAxis);
    if (axisIndex < 0)
        return false;

    DragAxisOrigin = primary->GetTransform().Location;
    DragAxisDirection = GizmoMath::GetAxisDirection(primary, axisIndex, ECoordSpace::Local);

    float axisT = 0.0f;
    if (!TryComputeAxisParameterFromMouse(e, ctx, DragAxisOrigin, DragAxisDirection, axisT))
    {
        ResetManipulationState();
        return false;
    }

    OriginalTransforms.reserve(primaries.size());
    for (USceneComponent* comp : primaries)
    {
        OriginalTransforms.push_back(comp->GetTransform());
    }

    ActiveAxis = pickedAxis;
    HoveredAxis = pickedAxis;
    bDragging = true;
    DragStartAxisT = axisT;
    bActiveUniformScale = ctx.Editor.Tools.GetUniformScale();

    return true;
}

void ScaleTool::OnMouseMove(const MouseEvent& e, AppContext& ctx)
{
    if (!bDragging)
    {
        EAxis hoveredAxis = EAxis::None;
        HoveredAxis = TryPickAxisHandle(e, ctx, hoveredAxis) ? hoveredAxis : EAxis::None;
        return;
    }

    TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
    if (primaries.empty() || primaries.size() != OriginalTransforms.size())
    {
        HoveredAxis = EAxis::None;
        OriginalTransforms.clear();
        ResetManipulationState();
        return;
    }

    float currentAxisT = 0.0f;
    if (!TryComputeAxisParameterFromMouse(e, ctx, DragAxisOrigin, DragAxisDirection, currentAxisT))
        return;

    const float delta = currentAxisT - DragStartAxisT;
    ApplyScaleDelta(primaries, delta);
}

void ScaleTool::OnMouseUp(const MouseEvent& e, AppContext& ctx)
{
    (void)e;

    if (!bDragging)
        return;

    TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
    if (primaries.empty() || primaries.size() != OriginalTransforms.size())
    {
        OriginalTransforms.clear();
        ResetManipulationState();
        return;
    }

    const int32 count = static_cast<int32>(primaries.size());
    for (int32 i = 0; i < count; ++i)
    {
        USceneComponent* primary = primaries[i];
        if (primary == nullptr)
            continue;

        const Transform originalTransform = OriginalTransforms[i];
        const Transform finalTransform = primary->GetTransform();

        primary->SetTransform(originalTransform);

        if (!NearlySameScale(finalTransform.Scale, originalTransform.Scale))
        {
            ctx.Dispatch(std::make_unique<SetTransformCommand>(primary, finalTransform));
        }
    }

    OriginalTransforms.clear();
    ResetManipulationState();
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

    const bool bUniformVisual = bDragging ? bActiveUniformScale : ctx.Editor.Tools.GetUniformScale();
    out.bUniformScale = bUniformVisual;

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

            out.Axes[axisIndex].BaseColor = bUniformVisual ? GizmoMath::AxisColorALL : colors[axisIndex];
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