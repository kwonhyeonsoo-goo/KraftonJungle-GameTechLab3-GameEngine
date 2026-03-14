#pragma once
#include "Panels/IEditorPanel.h"

class EditorManager {
public:
    EditorManager();

    void Register(IEditorPanel* panel);
    void RenderAll(AppContext& ctx);

    IEditorPanel* Find(const FString& name) const;

private:
    TArray<IEditorPanel*> Panels;
};