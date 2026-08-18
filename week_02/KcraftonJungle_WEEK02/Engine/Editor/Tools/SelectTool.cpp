#include "SelectTool.h"

void SelectTool::OnMouseDown(const MouseEvent& e, AppContext& ctx)
{
    if (!e.LeftDown)
        return;

    const FMatrix invViewProj = ctx.Editor.GetActiveViewport().Projection.Mode == EEditorProjectionMode::Orthographic ?
                                ctx.Editor.GetViewOrthoMatrix().Inversed() :
                                ctx.Editor.GetViewProjMatrix().Inversed();

    const Ray ray = PickingService::ScreenToRay(
        e.X, e.Y,
        ctx.Window.GetWidth(),
        ctx.Window.GetHeight(),
        invViewProj);

    UPrimitiveComponent* hit = PickingService::Pick(ray, ctx.CurrentWorld.GetAllObjects());

    if (hit != nullptr)
    {
        const bool additive = e.Ctrl || e.Shift;
        ctx.Editor.Selection.Select(hit, additive);
    }
    else
    {
        if (!(e.Ctrl || e.Shift))
        {
            ctx.Editor.Selection.Clear();
        }
    }
}