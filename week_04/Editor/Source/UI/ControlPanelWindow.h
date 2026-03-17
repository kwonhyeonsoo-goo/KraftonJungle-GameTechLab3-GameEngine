#pragma once
#include "CoreMinimal.h"

class CCore;
class AActor;

class CControlPanelWindow
{
public:
	void Render(CCore* Core, AActor*& SelectedActor);

private:
	TArray<FString> SceneFiles;
	int32 SelectedSceneIndex = -1;
};
