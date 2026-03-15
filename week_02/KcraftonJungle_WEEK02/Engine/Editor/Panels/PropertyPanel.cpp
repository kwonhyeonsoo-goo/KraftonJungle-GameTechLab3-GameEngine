#include "PropertyPanel.h"

void PropertyPanel::OnRender(AppContext& ctx)
{
    ImGui::Begin("Property");

    if (Current == nullptr) {
        ImGui::Text("No selection");
        ImGui::End();
        return;
    }

    UObject* obj = ctx.Objects.Find(Current->GetUUID());
    if (obj == nullptr || !obj->IsA<USceneComponent>()) {
        Current = nullptr;
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

        ctx.Dispatch(new SetTransformCommand(sceneComp, newT));
    }

    ImGui::End();
}

void PropertyPanel::OnObjectDestroyed(const ObjectDestroyedEvent& e)
{
	if (Current != nullptr && e.UUID == Current->GetUUID())
		Current = nullptr;
}

void PropertyPanel::OnSelectionChanged(const SelectionChangedEvent& e)
{
	Current = e.Primary;
}
