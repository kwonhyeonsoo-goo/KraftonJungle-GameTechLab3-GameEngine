#pragma once

#include "CoreMinimal.h"
#include "imgui.h"

class CConsoleWindow
{
public:
	CConsoleWindow();

	void Render();
	void AddLog(const char* Format, ...);
	void Clear();

private:
	TArray<FString> Items;
	ImGuiTextFilter Filter;
	bool AutoScroll;
	bool ScrollToBottom;
};
