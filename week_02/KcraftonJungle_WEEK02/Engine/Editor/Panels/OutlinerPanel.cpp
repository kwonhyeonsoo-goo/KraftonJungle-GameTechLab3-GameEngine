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

    float w = (float)ctx.Window.GetWidth();
    float h = (float)ctx.Window.GetHeight();
    float leftWidth = w * 0.2f;
    float statHeight = h * 0.45f;
    float consoleHeight = h * 0.2f;
    float outlinerHeight = h - statHeight - consoleHeight;

    ImGui::SetNextWindowPos(ImVec2(0, statHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftWidth, outlinerHeight), ImGuiCond_Always);
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

            bool isSelected = IsUUIDHighlighted(HighlightedUUIDs, prim->GetUUID());

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

            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                ctx.Editor.FocusObject(prim, 5.0f);
                bool additive = ImGui::GetIO().KeyCtrl;
                ctx.Editor.Selection.Select(prim, additive);
            }
            else if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
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
    HighlightedUUIDs.clear();

    for(auto prim : e.Primitives)
    {
        HighlightedUUIDs.push_back(prim->GetUUID());
	}
}

void OutlinerPanel::OnObjectDestroyed(const ObjectDestroyedEvent& e)
{
    for(auto it = HighlightedUUIDs.begin(); it != HighlightedUUIDs.end(); )
    {
        if (*it == e.UUID)
            it = HighlightedUUIDs.erase(it);
        else
            ++it;
	}
}