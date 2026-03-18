#include "ToolbarPanel.h"
#include <iostream>
#include "../Commands/SpawnObjectCommand.h"
#include "../Tools/TranslateTool.h"
#include "../Tools/RotateTool.h"
#include "../Tools/ScaleTool.h"
#include "../../Services/SerializationService.h"

void ToolbarPanel::OnRender(AppContext& ctx)
{
    if (!bVisible)
        return;
    float w = (float)ctx.Window.GetWidth();
    float h = (float)ctx.Window.GetHeight();
    float rightWidth = w * 0.2f;
    float consoleHeight = h * 0.2f;
    float propertyHeight = (h - consoleHeight) * 0.5f;
    float toolbarHeight = h - consoleHeight - propertyHeight;

    // property ¾Æ·¡
    ImGui::SetNextWindowPos(ImVec2(w - rightWidth, propertyHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightWidth, toolbarHeight), ImGuiCond_Always);

    if (!ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::End();
        return;
    }

    const ETransformMode CurrentMode = ctx.Editor.Tools.GetMode();

    ImGui::Text("Transform Mode");
    ImGui::Separator();

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

#pragma region SpawnObject
    ImGui::Spacing();
    ImGui::Text("Coord Space");
    ImGui::Separator();

    {
        const bool bLocal = ctx.Editor.Tools.GetCoordSpace() == ECoordSpace::Local;
        if (ImGui::Button(bLocal ? "Local" : "World"))
            ctx.Editor.Tools.SetCoordSpace(bLocal ? ECoordSpace::World : ECoordSpace::Local);
    }

    ImGui::Spacing();
    ImGui::Text("View");
    ImGui::Separator();
    FEditorViewport& Viewport = ctx.Editor.GetActiveViewport();
    EEditorProjectionMode& Mode = Viewport.Projection.Mode;

    bool bPerspective = (Mode == EEditorProjectionMode::Perspective);
    bool bOrthographic = (Mode == EEditorProjectionMode::Orthographic);

    if (ImGui::RadioButton("Perspective", bPerspective))
    {
        Mode = EEditorProjectionMode::Perspective;
    }
    ImGui::SameLine();

    if (ImGui::RadioButton("Orthographic", bOrthographic))
    {
        Mode = EEditorProjectionMode::Orthographic;
    }
    ImGui::Spacing();
    ImGui::Text("Spawn Object");
    ImGui::Separator();

    const Transform DefaultSpawnTransform = Transform::FromEulerDegrees(
        FVector(0.0f, 0.0f, 0.0f),
        FVector(0.0f, 0.0f, 0.0f),
        FVector(1.0f, 1.0f, 1.0f)
    );

    const char* ItemNames[] = {
        "Cube",
        "Sphere",
        "Plane",
        "Triangle"
    };
    static int32 currentItem = 0;
    static int32 NumItem = 1;

    ImGui::Combo("Object", &currentItem, ItemNames, IM_ARRAYSIZE(ItemNames));

    if (ImGui::Button("Spawn"))
    {
        for (int i = 0; i < NumItem; ++i)
        {
            ctx.Dispatch(std::make_unique<SpawnObjectCommand>(
                ctx,
                (EPrimitiveShape)currentItem,
                DefaultSpawnTransform
            ));
        }
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::DragInt("##NumOfSpawn", &NumItem, 1, 1, 1000); // ## ·Î ¶óº§ ¼û±è
    ImGui::Separator();
#pragma endregion

#pragma region SceneFeature
    ImGui::Spacing();
    ImGui::Text("Scene");
    ImGui::Separator();

    static char NameBuf[256] = {};

    ImGui::Text("Scene Name : %s", ctx.CurrentWorld.GetName().c_str());

    if (!ImGui::IsItemActive())
    {
        strncpy_s(NameBuf, ctx.CurrentWorld.GetName().c_str(), sizeof(NameBuf));
    }

    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::InputText("##SceneName", NameBuf, sizeof(NameBuf),
        ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (NameBuf[0] == '\0')
            strncpy_s(NameBuf, "default", sizeof(NameBuf));
        ctx.CurrentWorld.SetName(FString(NameBuf));
    }

    if (ImGui::Button("New Scene"))
    {
        ctx.NewScene();
        std::cout << "Clear" << std::endl;
    }
    ImGui::SameLine();

    if (ImGui::Button("Save Scene"))
    {
        std::cout << "Save Start" << std::endl;

        if (SerializationService::Save(ctx)) {
            std::cout << "Save Sucess" << std::endl;
        }

        std::cout << "Save End" << std::endl;
    }
    ImGui::SameLine();

    if (ImGui::Button("Load Scene"))
    {
        std::cout << "Load Start" << std::endl;

        if (SerializationService::Load(ctx)) {
            std::cout << "Load Sucess" << std::endl;
        }

        std::cout << "Load End" << std::endl;
    }
#pragma endregion

    ImGui::End();
}
