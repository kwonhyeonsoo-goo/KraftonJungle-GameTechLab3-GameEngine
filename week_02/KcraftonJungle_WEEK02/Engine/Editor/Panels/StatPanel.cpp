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
        ImGui::Text("FPS                 : %.1f", 1.0f / ctx.Window.GetCurrentDeltaTime());
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Text("Object Count        : %u", Stats.ObjectCount);
        ImGui::Text("Selected Count      : %u", Stats.SelectedObjectCount);
        ImGui::Text("Total Alloc Bytes   : %u", Stats.TotalAllocBytes);
        ImGui::Text("Total Alloc Count   : %u", Stats.TotalAllocCount);

        ImGui::Spacing();
        ImGui::Separator();
        //ImGui::Text("Current Tool        : %s",
        //    ctx.Editor.Tools.GetActiveTool() ? ctx.Editor.Tools.GetActiveTool()->GetName().c_str() : "None");

        ImGui::Text("Transform Mode      : %s",
            ctx.Editor.Tools.GetMode() == ETransformMode::Translate ? "Translate" :
            ctx.Editor.Tools.GetMode() == ETransformMode::Rotate ? "Rotate" :
            "Scale");

        ImGui::Text("Coord Space         : %s",
            ctx.Editor.Tools.GetCoordSpace() == ECoordSpace::World ? "World" : "Local");

        //ImGui::Text("Snap Enabled        : %s",
        //    ctx.Editor.Tools.IsSnapEnabled() ? "Yes" : "No");

        //ImGui::Text("Snap Value          : %.2f", ctx.Editor.Tools.GetSnapValue());

        FEditorCameraState& Cam = ctx.Editor.GetActiveCamera();

        float YawDeg = Cam.GetYawDeg();
        float PitchDeg = Cam.GetPitchDeg();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Camera");
        ImGui::DragFloat3("Position", &Cam.Position.x, 0.01f);
        if (ImGui::DragFloat("Yaw", &YawDeg, 0.1f, -180.f, 180.f))
        {
            Cam.SetYawDeg(YawDeg);
        }
        if (ImGui::DragFloat("Pitch", &PitchDeg, 0.1f, -89.f, 89.f))
        {
            Cam.SetPitchDeg(PitchDeg);
        }
        ImGui::DragFloat("Move Speed", &Cam.MoveSpeed, 0.01f, 0.1f, 50.f);
        ImGui::DragFloat("Rot Speed", &Cam.RotSpeed, 0.001f, 0.01f, 5.f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Projection");

        ImGui::DragFloat("FovY",    &ctx.Editor.GetActiveViewport().Projection.FovY, 0.1f, 10.f, 170.f, "%.2f");
        ImGui::DragFloat("Near",    &ctx.Editor.GetActiveViewport().Projection.NearZ, 0.001f, 0.001f, 10.f, "%.3f");
        ImGui::DragFloat("Far",     &ctx.Editor.GetActiveViewport().Projection.FarZ, 1.f, 10.f, 10000.f, "%.1f");
    }

    ImGui::End();
}