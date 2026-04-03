#pragma once

#include "Types/Array.h"
#include "Types/PlatformTypes.h"

class FCamera;
class FScene;

struct FVisibilityResults
{
	uint64 FrameNumber = 0;
	TArray<uint32> VisiblePrimitiveIndices;
};

class FVisibilitySystem
{
public:
	void Reset();
	void Build(const FScene& InScene, const FCamera& InCamera, FVisibilityResults& OutResults);

private:
	uint64 NextFrameNumber = 1;
};
