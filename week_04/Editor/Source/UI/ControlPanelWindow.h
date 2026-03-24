#pragma once
#include "CoreMinimal.h"

class CCore;

class CControlPanelWindow
{
public:
	void Render(CCore* Core);
	bool IsSpawnTextInputActive() const { return bSpawnTextInputActive; }

private:
	TArray<FString> SceneFiles;
	int32 SelectedSceneIndex = -1;
	bool bSpawnTextInputActive = false;
};
