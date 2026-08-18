#pragma once
#include "IEditorPanel.h"
#include "../../Services/StatsService.h"
#include "../../../AppContext.h"
#include "../../../Editor/ImGui/imgui.h"

class StatPanel : public IEditorPanel {
public:
    void    OnRender(AppContext& ctx) override;
    FString GetName() const override { return "Stats"; }
};