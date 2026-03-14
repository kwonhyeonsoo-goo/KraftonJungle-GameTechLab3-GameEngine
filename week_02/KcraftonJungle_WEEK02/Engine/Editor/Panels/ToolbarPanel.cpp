#include "ToolbarPanel.h"

#include "ToolbarPanel.h"


void ToolbarPanel::OnRender(AppContext& ctx)
{
    if (!bVisible)
        return;

    if (!ImGui::Begin("Toolbar", &bVisible,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::End();
        return;
    }

    const ETransformMode CurrentMode = ctx.Editor.Tools.GetMode();

    ImGui::Text("Transform Mode");
    ImGui::Separator();

    // Translate
    if (CurrentMode == ETransformMode::Translate)
    {
        ImGui::BeginDisabled();
        ImGui::Button("Translate");
        ImGui::EndDisabled();
    }
    else
    {
        if (ImGui::Button("Translate"))
        {
            ctx.Editor.Tools.SetMode(ETransformMode::Translate);
        }
    }

    ImGui::SameLine();

    if (CurrentMode == ETransformMode::Rotate)
    {
        ImGui::BeginDisabled();
        ImGui::Button("Rotate");
        ImGui::EndDisabled();
    }
    else
    {
        if (ImGui::Button("Rotate"))
        {
            ctx.Editor.Tools.SetMode(ETransformMode::Rotate);
        }
    }

    ImGui::SameLine();

    if (CurrentMode == ETransformMode::Scale)
    {
        ImGui::BeginDisabled();
        ImGui::Button("Scale");
        ImGui::EndDisabled();
    }
    else
    {
        if (ImGui::Button("Scale"))
        {
            ctx.Editor.Tools.SetMode(ETransformMode::Scale);
        }
    }

    ImGui::Spacing();
    ImGui::Text("Spawn Object");
    ImGui::Separator();

    const Transform DefaultSpawnTransform(
        FVector(0.0f, 0.0f, 0.0f),
        FVector(0.0f, 0.0f, 0.0f),
        FVector(1.0f, 1.0f, 1.0f)
    );

    if (ImGui::Button("Spawn Cube"))
    {
        ctx.Dispatch(new SpawnObjectCommand(
            ctx,
            EPrimitiveShape::Cube,
            DefaultSpawnTransform
        ));
    }

    ImGui::SameLine();

    if (ImGui::Button("Spawn Sphere"))
    {
        ctx.Dispatch(new SpawnObjectCommand(
            ctx,
            EPrimitiveShape::Sphere,
            DefaultSpawnTransform
        ));
    }

    ImGui::SameLine();

    if (ImGui::Button("Spawn Plane"))
    {
        ctx.Dispatch(new SpawnObjectCommand(
            ctx,
            EPrimitiveShape::Plane,
            DefaultSpawnTransform
        ));
    }

    ImGui::SameLine();

    if (ImGui::Button("Spawn Triangle"))
    {
        ctx.Dispatch(new SpawnObjectCommand(
            ctx,
            EPrimitiveShape::Triangle,
            DefaultSpawnTransform
        ));
    }

    ImGui::End();
}