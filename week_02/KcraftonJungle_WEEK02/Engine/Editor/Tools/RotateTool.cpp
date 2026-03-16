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

int RotateTool::AxisToIndex(EAxis axis)
{
    switch (axis)
    {
    case EAxis::X: return 0;
    case EAxis::Y: return 1;
    case EAxis::Z: return 2;
    default:       return -1;
    }
}

RotateTool::EAxis RotateTool::IndexToAxis(int index)
{
    switch (index)
    {
    case 0: return EAxis::X;
    case 1: return EAxis::Y;
    case 2: return EAxis::Z;
    default:return EAxis::None;
    }
}

bool RotateTool::TryBeginManipulation(const MouseEvent& e, AppContext& ctx)
{
    if (!e.LeftDown)
        return false;

    ActiveAxis = EAxis::None;
    bDragging = false;
    DragStartDir = FVector::Zero;

    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (primary == nullptr)
        return false;

    const FVector center = primary->GetTransform().Location;
    const ECoordSpace coordSpace = ctx.Editor.Tools.GetCoordSpace();

    const FMatrix viewProj = ctx.Editor.bOrthoMode
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
    OriginalTransform = primary->GetTransform();

    return true;
}

void RotateTool::OnMouseMove(const MouseEvent& e, AppContext& ctx)
{
    if (!bDragging)
        return;

    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (primary == nullptr)
    {
        ActiveAxis = EAxis::None;
        bDragging = false;
        DragStartDir = FVector::Zero;
        return;
    }

    const int axisIndex = AxisToIndex(ActiveAxis);
    if (axisIndex < 0)
        return;

    const FVector center = OriginalTransform.Location;
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
    
    const Quatanian Origin(OriginalTransform.ToQuaternion());
    const Quatanian Delta = MakeAxisAngleQuat(axis, angleRad);
    FVector4 NewQuatanian = NormalizeQuat(Delta.Mul(Origin));
    
    Transform NewRotation = OriginalTransform;
    NewRotation.Rotation = Transform::ToEularAngle(NewQuatanian);

    primary->SetTransform(NewRotation);
}

void RotateTool::OnMouseUp(const MouseEvent& e, AppContext& ctx)
{
    (void)e;

    if (!bDragging)
        return;

    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (primary != nullptr)
    {
        const Transform finalTransform = primary->GetTransform();

        if (!NearlySameRotation(finalTransform.Rotation, OriginalTransform.Rotation))
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
    DragStartDir = FVector::Zero;
}

void RotateTool::BuildGizmoOverlay(AppContext& ctx, RenderQueue& queue)
{
    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (primary == nullptr)
        return;

    const FVector center = primary->GetTransform().Location;
    const ECoordSpace coordSpace = ctx.Editor.Tools.GetCoordSpace();
    const uint32 objectId = primary->GetUUID();

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const FVector normal = GizmoMath::GetAxisDirection(primary, axisIndex, coordSpace);
        const FVector axis1 = GizmoMath::MakeStablePerpendicular(normal);
        const FVector axis2 = normal.Cross(axis1).Normalized();

        const uint32 color =
            axisIndex == 0 ? GizmoMath::AxisColorX :
            axisIndex == 1 ? GizmoMath::AxisColorY :
            GizmoMath::AxisColorZ;

        // Torus mesh has its ring normal along +Z.
        // Build a rotation that maps local +Z -> normal, +X -> axis1, +Y -> axis2.
        // Row-vector convention: row i = where local basis vector i goes in world space.
        FMatrix axisRotation = FMatrix::Identity();
        axisRotation.M[0][0] = axis1.x;  axisRotation.M[0][1] = axis1.y;  axisRotation.M[0][2] = axis1.z;
        axisRotation.M[1][0] = axis2.x;  axisRotation.M[1][1] = axis2.y;  axisRotation.M[1][2] = axis2.z;
        axisRotation.M[2][0] = normal.x; axisRotation.M[2][1] = normal.y; axisRotation.M[2][2] = normal.z;

        const float gs = GizmoMath::ComputeGizmoScale(center, ctx);

        RenderCommand cmd = {};
        cmd.Type = ERenderType::Torus;
        cmd.WorldTransform = FMatrix::Scale(FVector(gs, gs, gs)) * axisRotation * FMatrix::Translation(center);
        cmd.ObjectId = objectId;
        cmd.Color = color;
        queue.Push(cmd);
    }
}