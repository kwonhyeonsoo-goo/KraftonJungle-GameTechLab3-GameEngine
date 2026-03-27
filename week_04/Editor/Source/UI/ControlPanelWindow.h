#pragma once
#include "CoreMinimal.h"

class CCore;

class CControlPanelWindow
{
public:
	void Render(CCore* Core);

private:
	TArray<FString> LevelFiles;
	int32 SelectedLevelIndex = -1;
};
