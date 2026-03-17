#pragma once
#include "IEditorPanel.h"
#include "../Events/EditorEvents.h"
#include "../../Foundation/Core/CoreTypes.h"

class OutlinerPanel : public IEditorPanel {
public:
    void OnRender(AppContext& ctx) override;
    FString GetName() const override { return "Outliner"; }

    void OnSelectionChanged(const SelectionChangedEvent& e);
    void OnObjectDestroyed(const ObjectDestroyedEvent& e);

    DelegateHandle SelectionChangedHandle = 0;
    DelegateHandle ObjectDestroyedHandle = 0;
private:
    TArray<uint32> HighlightedUUIDs;
};

inline bool IsUUIDHighlighted(const TArray<uint32>& highlightedUUIDs, uint32 uuid)
{
    for (uint32 highlighted : highlightedUUIDs)
    {
        if (highlighted == uuid)
            return true;
    }
    return false;
}