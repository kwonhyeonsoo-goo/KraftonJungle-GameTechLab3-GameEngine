#pragma once
#include "CoreMinimal.h"

class FCore;

class FControlPanelWindow
{
public:
	void Render(FCore* Core);

private:
	TArray<FString> LevelFiles;
	int32 SelectedLevelIndex = -1;
};
