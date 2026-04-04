#pragma once

#include "ThirdParty/ImGui/imgui.h"

class FCore;

class FEditorControlPanel
{
public:
	FEditorControlPanel(FCore* InCore);
	~FEditorControlPanel();

public:
	void Render();

private:
	FCore* Core = nullptr;
};