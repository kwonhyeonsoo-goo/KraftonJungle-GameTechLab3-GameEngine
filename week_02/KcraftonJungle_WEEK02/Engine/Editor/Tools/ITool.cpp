#include "ITool.h"
#include "../../World/USceneComponent.h"
#include "../../../AppContext.h"
#include "../../Rendering/RenderTypes.h"
#include "GizmoMath.h"

void ITool::FillGizmoState(AppContext& ctx, GizmoState& out) const
{
    USceneComponent* primary = ctx.Editor.Selection.GetPrimary();
    if (!primary) return;

    const FVector origin = primary->GetTransform().Location;
    const float   gs = GizmoMath::ComputeGizmoScale(origin, ctx);
    const ECoordSpace space = ctx.Editor.Tools.GetCoordSpace();

    out.bActive = true;
    out.HoveredAxisIndex = bDragging ? -1 : AxisToIndex(HoveredAxis);
    out.ActiveAxisIndex = bDragging ? AxisToIndex(ActiveAxis) : -1;

    static const uint32 colors[3] = {
        GizmoMath::AxisColorX, GizmoMath::AxisColorY, GizmoMath::AxisColorZ
    };

    for (int i = 0; i < 3; ++i)
    {
        FVector axisDir = GizmoMath::GetAxisDirection(primary, i, space);
        out.Axes[i].BaseColor = colors[i];
        FillAxisData(origin, axisDir, gs, i, out.Axes[i]);
    }
}

int ITool::AxisToIndex(EAxis axis)
{
    switch (axis)
    {
    case EAxis::X: return 0;
    case EAxis::Y: return 1;
    case EAxis::Z: return 2;
    default:       return -1;
    }
}

ITool::EAxis ITool::IndexToAxis(int index)
{
    switch (index)
    {
    case 0: return EAxis::X;
    case 1: return EAxis::Y;
    case 2: return EAxis::Z;
    default:return EAxis::None;
    }
}