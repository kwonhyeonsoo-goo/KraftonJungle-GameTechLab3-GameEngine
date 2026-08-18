#pragma once
#include "IEditorPanel.h"
#include "../../Foundation/Core/CoreTypes.h"
#include "../../../AppContext.h"
#include "../../../Editor/ImGui/imgui.h"

#include "../ToolContext.h"
#include "../Commands/SpawnObjectCommand.h"
#include "../../World/Transform.h"
#include "../../Foundation/Math/FVector.h"

class ToolbarPanel : public IEditorPanel {
public:
    void    OnRender(AppContext& ctx) override;
    FString GetName() const override { return "Toolbar"; }
};