#pragma once

#include "Types/PlatformTypes.h"

struct FPickState;

class FStatsSystem
{
public:
	void Reset();
	void BeginFrame();
	void EndFrame();
	void ApplyPickState(const FPickState& InPickState);

	double GetFrameTimeMs() const { return FrameTimeMs; }
	double GetFramesPerSecond() const { return FramesPerSecond; }
	double GetLastPickTimeMs() const { return LastPickTimeMs; }
	double GetTotalPickTimeMs() const { return TotalPickTimeMs; }
	uint64 GetTotalPickCount() const { return TotalPickCount; }
	uint64 GetFrameNumber() const { return FrameNumber; }

private:
	uint64 LastFrameCounter = 0;
	double FrameTimeMs = 0.0;
	double FramesPerSecond = 0.0;
	double LastPickTimeMs = 0.0;
	double TotalPickTimeMs = 0.0;
	uint64 TotalPickCount = 0;
	uint64 FrameNumber = 0;
};
