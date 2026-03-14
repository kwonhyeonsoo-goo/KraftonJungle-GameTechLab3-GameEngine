#pragma once

#include "CoreMinimal.h"
#include "imgui.h"

class AActor;

class CPropertyWindow
{
public:
	void Render(AActor* SelectedActor);
};
