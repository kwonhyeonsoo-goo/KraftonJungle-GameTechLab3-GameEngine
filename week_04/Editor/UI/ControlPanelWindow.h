#pragma once

#include "CoreMinimal.h"
#include "imgui.h"

class CCore;
class AActor;

class CControlPanelWindow
{
public:
	// Render returns true if SelectedActor was changed
	void Render(CCore* Core, AActor*& SelectedActor);
};
