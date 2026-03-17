#include "RotateTool.h"

namespace
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr int RingSamples = 96;

    bool NearlySameRotation(const FVector& a, const FVector& b, float eps = 0.0001f)
    {
        return std::fabs(a.x - b.x) <= eps
            && std::fabs(a.y - b.y) <= eps
            && std::fabs(a.z - b.z) <= eps;
    }
}

bool RotateTool::TryBeginManipulation(const MouseEvent& e, AppContext& ctx)
{
    if (!e.LeftDown)
        return false;

    ActiveAxis = EAxis::None;
    bDragging = false;
    DragStartDir = FVector::Zero;

    TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
    if (primaries.empty())
        return false;

    USceneComponent* primary = primaries.back();

    const FVector center = primary->GetTransform().Location;
    const ECoordSpace coordSpace = ctx.Editor.Tools.GetCoordSpace();

    const FMatrix viewProj = ctx.Editor.GetActiveViewport().Projection.Mode == EEditorProjectionMode::Orthographic
        ? ctx.Editor.GetViewOrthoMatrix()
        : ctx.Editor.GetViewProjMatrix();
    const int32 viewportW = ctx.Window.GetWidth();
    const int32 viewportH = ctx.Window.GetHeight();
    const FVector2D mouse2D((float)e.X, (float)e.Y);

    float bestDistance = GizmoMath::GizmoPickThresholdPx;
    EAxis bestAxis = EAxis::None;

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const FVector normal = GizmoMath::GetAxisDirection(primary, axisIndex, coordSpace);

        const float gizmoScale = GizmoMath::ComputeGizmoScale(center, ctx);

        const TArray<FVector2D> ring2D = GizmoMath::SampleRing2D(
            center,
            normal,
            GizmoMath::GizmoRingRadius * gizmoScale,
            RingSamples,
            viewProj,
            viewportW,
            viewportH);

        const float dist = GizmoMath::DistancePointToPolyline2D(mouse2D, ring2D);
        if (dist < bestDistance)
        {
            bestDistance = dist;
            bestAxis = IndexToAxis(axisIndex);
        }
    }

    if (bestAxis == EAxis::None)
        return false;

    const int axisIndex = AxisToIndex(bestAxis);
    const FVector axis = GizmoMath::GetAxisDirection(primary, axisIndex, coordSpace);
    const Ray ray = GizmoMath::BuildMouseRay(e, ctx);

    FVector hitPoint;
    if (!GizmoMath::RayPlaneIntersection(ray, center, axis, hitPoint))
        return false;

    const FVector startDir = (hitPoint - center).Normalized();
    if (startDir.IsNearlyZero())
        return false;

    ActiveAxis = bestAxis;
    bDragging = true;
    DragStartDir = startDir;

    OriginalTransforms.clear();
    for (auto& comp : primaries) {
        OriginalTransforms.push_back(comp->GetTransform());
    }

    return true;
}

void RotateTool::OnMouseMove(const MouseEvent& e, AppContext& ctx)
{
    if (!bDragging)
    {
        TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
        if (primaries.empty()) { HoveredAxis = EAxis::None; return; }

        // 호버 감지
        USceneComponent* primary = primaries.back();
        
        const FVector center = primary->GetTransform().Location;
        const ECoordSpace coordSpace = ctx.Editor.Tools.GetCoordSpace();
        const FMatrix viewProj = ctx.Editor.GetActiveViewport().Projection.Mode == EEditorProjectionMode::Orthographic
            ? ctx.Editor.GetViewOrthoMatrix()
            : ctx.Editor.GetViewProjMatrix();
        const FVector2D mouse2D((float)e.X, (float)e.Y);
        const float gs = GizmoMath::ComputeGizmoScale(center, ctx);

        float bestDist = GizmoMath::GizmoPickThresholdPx;
        EAxis bestAxis = EAxis::None;

        for (int i = 0; i < 3; ++i)
        {
            const FVector normal = GizmoMath::GetAxisDirection(primary, i, coordSpace);
            const TArray<FVector2D> ring2D = GizmoMath::SampleRing2D(
                center, normal, GizmoMath::GizmoRingRadius * gs,
                RingSamples, viewProj, ctx.Window.GetWidth(), ctx.Window.GetHeight());
            const float dist = GizmoMath::DistancePointToPolyline2D(mouse2D, ring2D);
            if (dist < bestDist) { bestDist = dist; bestAxis = IndexToAxis(i); }
        }

        HoveredAxis = bestAxis;
        return;
    }

    TArray<USceneComponent*> primaries = ctx.Editor.Selection.GetAll();
    if (primaries.empty()) {
        ActiveAxis = EAxis::None;
        bDragging = false;
        DragStartDir = FVector::Zero;
		return; 
    }

    USceneComponent* primary = primaries.back();

    const int axisIndex = AxisToIndex(ActiveAxis);
    if (axisIndex < 0)
        return;

    const FVector center = OriginalTransforms.back().Location;
    const ECoordSpace coordSpace = ctx.Editor.Tools.GetCoordSpace();
    const FVector axis = GizmoMath::GetAxisDirection(primary, axisIndex, coordSpace);

    const Ray ray = GizmoMath::BuildMouseRay(e, ctx);

    FVector hitPoint;
    if (!GizmoMath::RayPlaneIntersection(ray, center, axis, hitPoint))
        return;

    const FVector currentDir = (hitPoint - center).Normalized();
    if (currentDir.IsNearlyZero())
        return;

    const float angleRad = GizmoMath::SignedAngleAroundAxis(DragStartDir, currentDir, axis);

    for (int32 i = 0; i < primaries.size(); i++) {
        USceneComponent* primary = primaries[i];

        const Quatanian Origin(OriginalTransforms[i].ToQuaternion());
        const Quatanian Delta = MakeAxisAngleQuat(axis, angleRad);
        FVector4 NewQuatanian = NormalizeQuat(Delta.Mul(Origin));

        FVector Rotation = Transform::ToEularAngle(NewQuatanian);

        Transform newTransform = OriginalTransforms[i];
        newTransform.Rotation = Rotation;
        primaries[i]->SetTransform(newTransform);
    }
}

void RotateTool::OnMouseUp(const MouseEvent& e, AppContext& ctx)
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

            if (!NearlySameRotation(finalTransform.Rotation, OriginalTransforms[i].Rotation))
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
    DragStartDir = FVector::Zero;
}

void RotateTool::FillAxisData(const FVector& origin, const FVector& axisDir,
                               float scale, int /*axisIndex*/, GizmoAxisData& out) const
{
    out.WorldTransform = GizmoMath::MakeAxisTransform(origin, axisDir, scale);
    out.RenderType = ERenderType::RotationGizmo;
}