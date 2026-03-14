#pragma once
#include "ITool.h"
#include "../../Services/PickingService.h"
#include "../SelectionSet.h"
#include "../../../AppContext.h"

class SelectTool : public ITool {
public:
    void OnMouseDown(const MouseEvent& e, AppContext& ctx) override;
    FString GetName() const override { return "Select"; }
};