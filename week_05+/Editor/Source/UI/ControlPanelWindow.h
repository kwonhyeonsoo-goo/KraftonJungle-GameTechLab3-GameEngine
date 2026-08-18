#pragma once
#include "CoreMinimal.h"

class FCore;
class FEditorViewportClient;

class FControlPanelWindow
{
public:
	void Render(FCore* Core, FEditorViewportClient* ActiveViewportClient);

private:
	TArray<FString> LevelFiles;
	int32 SelectedLevelIndex = -1;
};
