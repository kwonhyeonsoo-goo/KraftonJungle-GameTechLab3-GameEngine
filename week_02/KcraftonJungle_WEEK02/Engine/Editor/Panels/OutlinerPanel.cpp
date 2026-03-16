#include "OutlinerPanel.h"
#include "../../../AppContext.h"
#include "../../World/UPrimitiveComponent.h"

static const char* GetShapeName(EPrimitiveShape shape)
{
    switch (shape)
    {
    case EPrimitiveShape::Cube:     return "Cube";
    case EPrimitiveShape::Sphere:   return "Sphere";
    case EPrimitiveShape::Plane:    return "Plane";
    case EPrimitiveShape::Triangle: return "Triangle";
    default:                        return "Unknown";
    }
}

void OutlinerPanel::OnRender(AppContext& ctx)
{
    ImGui::Begin("Outliner");

    bool levelOpen = ImGui::TreeNodeEx(
        ctx.CurrentWorld.GetName().c_str(),
        ImGuiTreeNodeFlags_DefaultOpen
    );

    if (levelOpen)
    {
        const auto& objects = ctx.Objects.GUObjectArray();

        for (int i = 0; i < (int)objects.size(); i++)
        {
            UObject* obj = objects[i].get();
            if (!obj->IsA<UPrimitiveComponent>()) continue;
            auto* prim = static_cast<UPrimitiveComponent*>(obj);
            if (!prim) continue;

            bool isSelected = (obj->GetUUID() == HighlightedUUID);

            char label[64];
            snprintf(label, sizeof(label), "%s_%u",
                GetShapeName(prim->Shape), obj->GetUUID());

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_Leaf |
                ImGuiTreeNodeFlags_NoTreePushOnOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth;

            if (isSelected)
                flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::TreeNodeEx((void*)(intptr_t)obj->GetUUID(), flags, label);

            if (ImGui::IsItemClicked())
            {
                bool additive = ImGui::GetIO().KeyCtrl;
                ctx.Editor.Selection.Select(prim, additive);
            }
        }

        ImGui::TreePop();
    }

    ImGui::End();
}

void OutlinerPanel::OnSelectionChanged(const SelectionChangedEvent& e)
{
    HighlightedUUID = e.Primary ? e.Primary->GetUUID() : 0;
}

void OutlinerPanel::OnObjectDestroyed(const ObjectDestroyedEvent& e)
{
    if (HighlightedUUID == e.UUID)
        HighlightedUUID = 0;
}