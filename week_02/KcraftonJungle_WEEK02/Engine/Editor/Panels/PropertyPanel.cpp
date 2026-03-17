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

    TArray<USceneComponent*> targets;
    for(auto & selected: CurrentObjs) {
        if (selected == nullptr) continue;

        UObject* obj = ctx.Objects.Find(selected->GetUUID());
        if (obj == nullptr || !obj->IsA<USceneComponent>()) continue;

        targets.push_back(static_cast<USceneComponent*>(obj));
    }

    if (targets.empty()) {
        CurrentObjs.clear();
        ImGui::Text("Invalid selection");
        ImGui::End();
        return;
    }

    USceneComponent* baseComp = targets.back();
    const Transform baseT = baseComp->GetTransform();

    float loc[3] = { baseT.Location.GetX(), baseT.Location.GetY(), baseT.Location.GetZ() };
    float rot[3] = { baseT.Rotation.GetX(), baseT.Rotation.GetY(), baseT.Rotation.GetZ() };
    float scale[3] = { baseT.Scale.GetX(), baseT.Scale.GetY(), baseT.Scale.GetZ() };

    const bool locChanged = ImGui::DragFloat3("Location", loc, 0.1f);
    const bool rotChanged = ImGui::DragFloat3("Rotation", rot, 1.0f);
    const bool sclChanged = ImGui::DragFloat3("Scale", scale, 0.1f);

    if (locChanged || rotChanged || sclChanged)
    {
        const FVector oldLoc = baseT.Location;
        const FVector oldRot = baseT.Rotation;
        const FVector oldScl = baseT.Scale;

        const FVector newLoc(loc[0], loc[1], loc[2]);
        const FVector newRot(rot[0], rot[1], rot[2]);   
        const FVector newScl(scale[0], scale[1], scale[2]);

        const FVector locDelta = newLoc - oldLoc;
        const FVector rotDelta = newRot - oldRot;
        const FVector sclDelta = newScl - oldScl;

        for (USceneComponent* target : targets)
        {
            Transform t = target->GetTransform();

            if (locChanged) t.Location = t.Location + locDelta;
            if (rotChanged) t.Rotation = t.Rotation + rotDelta;
            if (sclChanged) t.Scale = t.Scale + sclDelta;

            ctx.Dispatch(std::make_unique<SetTransformCommand>(target, t));
        }
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
