#include "StatPanel.h"

void StatPanel::OnRender(AppContext& ctx)
{
    if (!bVisible)
        return;

    const EngineStats Stats = StatsService::Collect(ctx);

    if (ImGui::Begin("Stats", &bVisible))
    {
        ImGui::Text("Engine Statistics");
        ImGui::Separator();

        ImGui::Text("Object Count        : %u", Stats.ObjectCount);
        ImGui::Text("Selected Count      : %u", Stats.SelectedObjectCount);
        ImGui::Text("Total Alloc Bytes   : %u", Stats.TotalAllocBytes);
        ImGui::Text("Total Alloc Count   : %u", Stats.TotalAllocCount);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Current Tool        : %s",
            ctx.Editor.Tools.GetActiveTool() ? ctx.Editor.Tools.GetActiveTool()->GetName().c_str() : "None");

        ImGui::Text("Transform Mode      : %s",
            ctx.Editor.Tools.GetMode() == ETransformMode::Translate ? "Translate" :
            ctx.Editor.Tools.GetMode() == ETransformMode::Rotate ? "Rotate" :
            "Scale");

        ImGui::Text("Coord Space         : %s",
            ctx.Editor.Tools.GetCoordSpace() == ECoordSpace::World ? "World" : "Local");

        ImGui::Text("Snap Enabled        : %s",
            ctx.Editor.Tools.IsSnapEnabled() ? "Yes" : "No");

        ImGui::Text("Snap Value          : %.2f", ctx.Editor.Tools.GetSnapValue());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Camera");

        ImGui::Text("Position            : (%.2f, %.2f, %.2f)",
            ctx.Editor.Camera.Position.GetX(),
            ctx.Editor.Camera.Position.GetY(),
            ctx.Editor.Camera.Position.GetZ());

        ImGui::Text("Yaw / Pitch         : %.2f / %.2f",
            ctx.Editor.Camera.Yaw,
            ctx.Editor.Camera.Pitch);

        ImGui::Text("Move Speed          : %.2f", ctx.Editor.Camera.MoveSpeed);
        ImGui::Text("Rot Speed           : %.2f", ctx.Editor.Camera.RotSpeed);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Projection");

        ImGui::Text("FovY                : %.2f", ctx.Editor.FovY);
        ImGui::Text("Aspect Ratio        : %.3f", ctx.Editor.AspectRatio);
        ImGui::Text("Near / Far          : %.3f / %.3f", ctx.Editor.NearZ, ctx.Editor.FarZ);
    }

    ImGui::End();
}