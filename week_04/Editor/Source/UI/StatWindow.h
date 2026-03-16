#pragma once
#include "CoreMinimal.h"
#include "imgui.h"

class CStatWindow
{
public:
	void Render();
	void SetObjectCount(uint32 InCount) { ObjectCount = InCount; }
	void SetHeapUsage(uint32 InBytes) { HeapUsageBytes = InBytes; }

private:
	uint32 ObjectCount = 0;
	uint32 HeapUsageBytes = 0;
};
