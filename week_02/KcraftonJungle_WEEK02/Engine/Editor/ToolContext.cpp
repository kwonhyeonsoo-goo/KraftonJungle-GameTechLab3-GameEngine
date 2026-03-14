#include "ToolContext.h"
#include "../Foundation/Core/Log.h"

ToolContext::ToolContext()
{
	Mode = ETransformMode::Translate;
	ActiveTool = nullptr;
	CoordSpace = ECoordSpace::World;
	SnapEnabled = false;
	SnapValue = 0.25f;
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
	if (Mode == mode) return;
	bool Switched = false;

	switch (mode)
	{
	case ETransformMode::Translate:
		Switched = ActivateTool("Translate");
		break;
	case ETransformMode::Rotate:
		Switched = ActivateTool("Rotate");
		break;
	case ETransformMode::Scale:
		Switched = ActivateTool("Scale");
		break;
	default:
		return;
	}
	if (Switched)
	{
		Mode = mode;
		OnModeChanged.Broadcast(Mode);
	}
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
	if (tool == nullptr) return;
	Tools[tool->GetName()] = tool;
}

bool ToolContext::ActivateTool(const FString& name)
{
	if (Tools.empty())
	{
		UE_LOG("Tools are not initalized.");
		return false;
	}

	auto it = Tools.find(name);
	if (it == Tools.end())
	{
		UE_LOG("Tool not found.");
		return false;
	}
	if (it->second == nullptr)
	{
		UE_LOG("Unknown Error.");
		return false;
	}
	ActiveTool = it->second;
	return true;
}