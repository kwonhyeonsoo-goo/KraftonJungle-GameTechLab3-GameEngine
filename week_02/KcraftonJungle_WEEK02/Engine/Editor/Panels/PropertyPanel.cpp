#include "PropertyPanel.h"
#include "PropertyPanel.h"

void PropertyPanel::OnRender(AppContext& ctx)
{
    ImGui::Begin("Property");

    if (CurrentObjs.empty()) {
        ImGui::Text("No selection");
        ImGui::End();
        return;
    }

    if (CurrentObjs.size() > 1) {
        ImGui::Text("Multi-selection (%d)", (int)CurrentObjs.size());
        ImGui::Text("Property editing is available for single selection only.");
        ImGui::End();
        return;
    }

    const USceneComponent* Current = CurrentObjs.back();

    UObject* obj = ctx.Objects.Find(Current->GetUUID());
    if (obj == nullptr || !obj->IsA<USceneComponent>()) {
        CurrentObjs.clear();
        ImGui::Text("Invalid selection");
        ImGui::End();
        return;
    }

    USceneComponent* sceneComp = static_cast<USceneComponent*>(obj);
    Transform t = sceneComp->GetTransform();

    float loc[3] = { t.Location.GetX(), t.Location.GetY(), t.Location.GetZ()};
    float rot[3] = { t.Rotation.GetX(), t.Rotation.GetY(), t.Rotation.GetZ()};
    float scale[3] = { t.Scale.GetX(), t.Scale.GetY(), t.Scale.GetZ()};

    bool changed = false;
    changed |= ImGui::DragFloat3("Location", loc, 0.1f);
    changed |= ImGui::DragFloat3("Rotation", rot, 1.0f);
    changed |= ImGui::DragFloat3("Scale", scale, 0.1f);

    if (changed) {
        Transform newT;
        newT.Location = FVector(loc[0], loc[1], loc[2]);
        newT.Rotation = FVector(rot[0], rot[1], rot[2]);
        newT.Scale = FVector(scale[0], scale[1], scale[2]);

        ctx.Dispatch(std::make_unique<SetTransformCommand>(sceneComp, newT));
    }

    ImGui::End();
}

void PropertyPanel::OnObjectDestroyed(const ObjectDestroyedEvent& e)
{
    for (auto it = CurrentObjs.begin(); it != CurrentObjs.end(); )
    {
        const USceneComponent* comp = *it;
        if (comp != nullptr && comp->GetUUID() == e.UUID)
            it = CurrentObjs.erase(it);
        else
            ++it;
    }
}

void PropertyPanel::OnSelectionChanged(const SelectionChangedEvent& e)
{
    CurrentObjs.clear();
    for (USceneComponent* comp : e.Primitives)
    {
        CurrentObjs.push_back(comp);
    }
}
