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
        ImGui::DragFloat3("Position", &ctx.Editor.Camera.Position.x, 0.01f);
        ImGui::DragFloat("Yaw", &ctx.Editor.Camera.Yaw, 0.1f);
        ImGui::DragFloat("Pitch", &ctx.Editor.Camera.Pitch, 0.1f, -89.f, 89.f);
        ImGui::DragFloat("Move Speed", &ctx.Editor.Camera.MoveSpeed, 0.01f, 0.1f, 50.f);
        ImGui::DragFloat("Rot Speed", &ctx.Editor.Camera.RotSpeed, 0.001f, 0.01f, 5.f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Projection");

        ImGui::DragFloat("FovY",    &ctx.Editor.FovY, 0.1f, 10.f, 170.f, "%.2f");
        ImGui::DragFloat("Near",    &ctx.Editor.NearZ, 0.001f, 0.001f, 10.f, "%.3f");
        ImGui::DragFloat("Far",     &ctx.Editor.FarZ, 1.f, 10.f, 10000.f, "%.1f");
    }

    ImGui::End();
}