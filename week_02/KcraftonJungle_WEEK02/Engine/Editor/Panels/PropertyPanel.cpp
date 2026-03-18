#include "PropertyPanel.h"
#include <algorithm>

void PropertyPanel::InvalidateRotationDisplayCache()
{
    bRotationDisplayValid = false;
    RotationDisplayTargetUUID = 0;
    RotationDisplayEulerDeg = FVector::Zero;
}

void PropertyPanel::OnRender(AppContext& ctx)
{
    float w = (float)ctx.Window.GetWidth();
    float h = (float)ctx.Window.GetHeight();
    float rightWidth = w * 0.2f;
    float consoleHeight = h * 0.2f;
    float propertyHeight = (h - consoleHeight) * 0.5f;

    // ¿ì»ó´Ü
    ImGui::SetNextWindowPos(ImVec2(w - rightWidth, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightWidth, propertyHeight), ImGuiCond_Always);
    ImGui::Begin("Property");

    if (CurrentObjs.empty()) {
        InvalidateRotationDisplayCache();
        ImGui::Text("No selection");
        ImGui::End();
        return;
    }

    TArray<USceneComponent*> targets;
    for (const USceneComponent* selected : CurrentObjs)
    {
        if (selected == nullptr) continue;

        UObject* obj = ctx.Objects.Find(selected->GetUUID());
        if (obj == nullptr || !obj->IsA<USceneComponent>()) continue;

        targets.push_back(static_cast<USceneComponent*>(obj));
    }

    if (targets.empty()) {
        CurrentObjs.clear();
        InvalidateRotationDisplayCache();
        ImGui::Text("Invalid selection");
        ImGui::End();
        return;
    }

    USceneComponent* baseComp = targets.back();
    const Transform baseT = baseComp->GetTransform();

    if (!bRotationDisplayValid || RotationDisplayTargetUUID != baseComp->GetUUID())
    {
        RotationDisplayTargetUUID = baseComp->GetUUID();
        RotationDisplayEulerDeg = baseT.GetEulerDegrees();
        bRotationDisplayValid = true;
    }
    else
    {
        RotationDisplayEulerDeg = baseT.GetEulerDegreesNearest(RotationDisplayEulerDeg);
    }

    const FVector oldBaseEulerDeg = RotationDisplayEulerDeg;

    float loc[3] = { baseT.Location.GetX(), baseT.Location.GetY(), baseT.Location.GetZ() };
    float rot[3] = { RotationDisplayEulerDeg.GetX(), RotationDisplayEulerDeg.GetY(), RotationDisplayEulerDeg.GetZ() };
    float scale[3] = { baseT.Scale.GetX(), baseT.Scale.GetY(), baseT.Scale.GetZ() };

    const bool locChanged = ImGui::DragFloat3("Location", loc, 0.1f);
    const bool rotChanged = ImGui::DragFloat3("Rotation (deg)", rot, 1.0f);
    const bool sclChanged = ImGui::DragFloat3("Scale", scale, 0.1f);

	bool UniformScale = ctx.Editor.Tools.GetUniformScale();
	ImGui::Checkbox("Uniform Sacle", &UniformScale);

	if (UniformScale != ctx.Editor.Tools.GetUniformScale()) {
		ctx.Editor.Tools.SetUniformScale(UniformScale);
	}

    if (locChanged || rotChanged || sclChanged)
    {
        const FVector oldLoc = baseT.Location;
        const FVector oldScl = baseT.Scale;

        const FVector newLoc(loc[0], loc[1], loc[2]);
        const FVector newBaseEulerDeg(rot[0], rot[1], rot[2]);
        const FVector newScl(scale[0], scale[1], scale[2]);

        const FVector locDelta = newLoc - oldLoc;
        FVector sclDelta = newScl - oldScl;

        if (UniformScale) {
            float ax = std::fabs(sclDelta.x);
            float ay = std::fabs(sclDelta.y);
            float az = std::fabs(sclDelta.z);

            float maxDelta = (std::max)(ax, (std::max)(ay, az));

            if (maxDelta > 0.0001f) {
                const float sign = (sclDelta.x >= 0.f && sclDelta.y >= 0.f && sclDelta.z >= 0.f) ? 1.f : -1.f;
                const FVector uniformDelta = FVector(maxDelta * sign, maxDelta * sign, maxDelta * sign);
                sclDelta.x = uniformDelta.x;
                sclDelta.y = uniformDelta.y;
                sclDelta.z = uniformDelta.z;
			}
        }

        FQuat deltaQuat = FQuat::Identity();
        if (rotChanged)
        {
            const FQuat oldBaseQuat = FQuat::FromEulerDegXYZ(oldBaseEulerDeg);
            const FQuat newBaseQuat = FQuat::FromEulerDegXYZ(newBaseEulerDeg);
            deltaQuat = (newBaseQuat * oldBaseQuat.Inversed()).Normalized();
        }

        for (USceneComponent* target : targets)
        {
            Transform t = target->GetTransform();

            if (locChanged) t.Location = t.Location + locDelta;
            if (rotChanged) t.Rotation = (deltaQuat * t.Rotation).Normalized();
            if (sclChanged) t.Scale = t.Scale + sclDelta;

            ctx.Dispatch(std::make_unique<SetTransformCommand>(target, t));
        }

        if (rotChanged)
        {
            RotationDisplayEulerDeg = newBaseEulerDeg;
            RotationDisplayTargetUUID = baseComp->GetUUID();
            bRotationDisplayValid = true;
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

    if (RotationDisplayTargetUUID == e.UUID)
    {
        InvalidateRotationDisplayCache();
    }
}

void PropertyPanel::OnSelectionChanged(const SelectionChangedEvent& e)
{
    CurrentObjs.clear();
    for (USceneComponent* comp : e.Primitives)
    {
        CurrentObjs.push_back(comp);
    }

    InvalidateRotationDisplayCache();
}
