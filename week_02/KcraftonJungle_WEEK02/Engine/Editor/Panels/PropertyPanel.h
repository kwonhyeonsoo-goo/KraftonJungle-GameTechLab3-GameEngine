#pragma once
#include "IEditorPanel.h"
#include "../Commands/SetTransformCommand.h"
#include "../../../AppContext.h"
#include "../../../Editor/ImGui/imgui.h"

class PropertyPanel : public IEditorPanel {
public:
    void OnRender(AppContext& ctx) override;
    FString GetName() const override { return "Property"; }

    void OnObjectDestroyed(const ObjectDestroyedEvent& e);
    void OnSelectionChanged(const SelectionChangedEvent& e);

    DelegateHandle SelectionChangedHandle = 0;
    DelegateHandle ObjectDestroyedHandle = 0;
private:
    void InvalidateRotationDisplayCache();

    TArray<const USceneComponent*> CurrentObjs;
    bool bRotationDisplayValid = false;
    uint32 RotationDisplayTargetUUID = 0;
    FVector RotationDisplayEulerDeg = FVector::Zero;
};
