#pragma once
#include "CoreMinimal.h"
#include "imgui.h"

struct FObjectEntry
{
	FString Name;
	FString ClassName;
	uint32 Size = 0;
};

class FStatWindow
{
public:
	void Render();
	void SetHeapUsage(uint32 InBytes) { HeapUsageBytes = InBytes; }

private:
	void RefreshObjectList();

	uint32 HeapUsageBytes = 0;

	TArray<FObjectEntry> ObjectEntries;
	bool bShowObjectList = false;
};
