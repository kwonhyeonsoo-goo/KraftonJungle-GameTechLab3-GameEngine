#pragma once
#include "CoreMinimal.h"
#include <string>
#include <vector>

class CCore;

class CControlPanelWindow
{
public:
	void Render(CCore* Core);

private:
	TArray<FString> SceneFiles;
	int32 SelectedSceneIndex = -1;
};
