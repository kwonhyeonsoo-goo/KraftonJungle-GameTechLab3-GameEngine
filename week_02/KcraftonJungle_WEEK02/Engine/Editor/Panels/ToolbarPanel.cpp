#include "ToolbarPanel.h"
#include <iostream>

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
    ImGui::Checkbox("Orthographic", &ctx.Editor.bOrthoMode);

    ImGui::Spacing();
    ImGui::Text("Spawn Object");
    ImGui::Separator();

    const Transform DefaultSpawnTransform(
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
    EPrimitiveShape MyEnum = EPrimitiveShape::Cube;
    static int32 currentItem = 0;
    static int32 NumItem = 1;

    ImGui::Combo("My Dropdown Label", &currentItem, ItemNames, IM_ARRAYSIZE(ItemNames));

    if (ImGui::Button("Spawn"))
    {
        for (int i = 0; i < NumItem; ++i)
        {
            ctx.Dispatch(new SpawnObjectCommand(
                ctx,
                (EPrimitiveShape)currentItem,
                DefaultSpawnTransform
            ));
        }
    }

    ImGui::SameLine();
    ImGui::DragInt("Num of spawn", &NumItem, 1, 1, 1000);
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

    if (ImGui::Button("Save Scene"))
    {
        std::cout << "Save Start" << std::endl;

        if (SerializationService::Save(ctx)) {
            std::cout << "Save Sucess" << std::endl;
        }

        std::cout << "Save End" << std::endl;
    }

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