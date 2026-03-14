#pragma once
#include "IEditorPanel.h"
#include "../../Foundation/Core/CoreTypes.h"
#include "../../Services/ConsoleService.h"
#include "../../../AppContext.h"

class ConsolePanel : public IEditorPanel {
public:
    void    OnRender(AppContext& ctx) override;
    FString GetName() const override { return "Console"; }

    // imgui_demo.cpp ShowExampleAppConsole Âü°í
private:
    char InputBuf[256] = {};
    bool ScrollToBottom = true;
};