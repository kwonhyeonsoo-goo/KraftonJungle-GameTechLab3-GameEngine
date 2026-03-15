#include "ToolContext.h"
#include "Tools/ITool.h"

ToolContext::ToolContext()
    : ActiveTool(nullptr)
    , Mode(ETransformMode::Translate)
    , CoordSpace(ECoordSpace::World)
    , SnapEnabled(false)
    , SnapValue(0.25f)
{
}

ITool* ToolContext::GetActiveTool() const
{
    return ActiveTool;
}

ETransformMode ToolContext::GetMode() const
{
    return Mode;
}

ECoordSpace ToolContext::GetCoordSpace() const
{
    return CoordSpace;
}

float ToolContext::GetSnapValue() const
{
    return SnapValue;
}

bool ToolContext::IsSnapEnabled() const
{
    return SnapEnabled;
}

void ToolContext::SetMode(ETransformMode mode)
{
    Mode = mode;

    switch (Mode)
    {
    case ETransformMode::Translate:
        ActivateTool("Translate");
        break;
    case ETransformMode::Rotate:
        ActivateTool("Rotate");
        break;
    case ETransformMode::Scale:
        ActivateTool("Scale");
        break;
    }

    OnModeChanged.Broadcast(Mode);
}

void ToolContext::SetCoordSpace(ECoordSpace cs)
{
    CoordSpace = cs;
}

void ToolContext::SetSnapEnabled(bool enabled)
{
    SnapEnabled = enabled;
}

void ToolContext::SetSnapValue(float value)
{
    SnapValue = value;
}

void ToolContext::RegisterTool(ITool* tool)
{
    if (tool == nullptr)
        return;

    Tools[tool->GetName()] = tool;
}

void ToolContext::ActivateTool(const FString& name)
{
    auto it = Tools.find(name);
    if (it != Tools.end())
    {
        ActiveTool = it->second;
    }
}