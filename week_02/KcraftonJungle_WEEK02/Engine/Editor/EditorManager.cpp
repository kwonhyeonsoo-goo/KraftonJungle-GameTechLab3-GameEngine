#include "EditorManager.h"

EditorManager::EditorManager()
{
}

void EditorManager::Register(IEditorPanel* panel)
{
    if (panel == nullptr) return;
    if (Find(panel->GetName()) != nullptr) return;
    Panels.push_back(panel);
}

void EditorManager::RenderAll(AppContext& ctx)
{
    for (auto it = Panels.begin(); it != Panels.end(); ++it)
    {
        IEditorPanel* panel = *it;
        if (panel == nullptr || !panel->bVisible) continue;
        panel->OnRender(ctx);
    }
}

IEditorPanel* EditorManager::Find(const FString& name) const
{
    for (auto it = Panels.begin(); it != Panels.end(); ++it)
    {
        IEditorPanel* panel = *it;
        if (panel == nullptr) continue;
        if (panel->GetName() == name) return panel;
    }

    return nullptr;
}