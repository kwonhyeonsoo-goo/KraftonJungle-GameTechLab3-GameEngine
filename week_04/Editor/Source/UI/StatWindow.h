#pragma once
#include "CoreMinimal.h"
#include "imgui.h"

class CStatWindow
{
public:
	void Render();
	void SetObjectCount(uint32 Count) { ObjectCount = Count; }
	void SetHeapUsage(uint32 Bytes) { HeapUsageBytes = Bytes; }

private:
	uint32 ObjectCount = 0;
	uint32 HeapUsageBytes = 0;
};
