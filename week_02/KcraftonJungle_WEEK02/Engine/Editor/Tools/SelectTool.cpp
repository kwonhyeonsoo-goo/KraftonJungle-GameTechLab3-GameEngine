#include "SelectTool.h"

void SelectTool::OnMouseDown(const MouseEvent& e, AppContext& ctx)
{
	Ray R = PickingService::ScreenToRay(e.X, e.Y, ctx.Window.GetWidth(), ctx.Window.GetHeight(), ctx.Editor.GetViewProjMatrix().Inversed());
	UPrimitiveComponent* hit = PickingService::Pick(R, ctx.CurrentScreen.GetAllObjects());
    if (hit)
    {
        ctx.Editor.Selection.Select(hit, e.Ctrl);
    }
    else
    {
        ctx.Editor.Selection.Clear();
    }
}
