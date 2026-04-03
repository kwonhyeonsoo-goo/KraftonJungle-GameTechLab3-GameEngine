#include "Stats/StatsSystem.h"

#include <algorithm>

#include <Windows.h>

#include "Picking/PickingSystem.h"

namespace
{
	uint64 QueryCycles64()
	{
		LARGE_INTEGER Counter = {};
		QueryPerformanceCounter(&Counter);
		return static_cast<uint64>(Counter.QuadPart);
	}

	double GetSecondsPerCycle()
	{
		static const double SecondsPerCycle = []()
		{
			LARGE_INTEGER Frequency = {};
			QueryPerformanceFrequency(&Frequency);
			return 1.0 / static_cast<double>(Frequency.QuadPart);
		}();
		return SecondsPerCycle;
	}
}

void FStatsSystem::Reset()
{
	LastFrameCounter = 0;
	FrameTimeMs = 0.0;
	FramesPerSecond = 0.0;
	LastPickTimeMs = 0.0;
	TotalPickTimeMs = 0.0;
	TotalPickCount = 0;
	FrameNumber = 0;
}

void FStatsSystem::BeginFrame()
{
	const uint64 CurrentFrameCounter = QueryCycles64();
	if (LastFrameCounter == 0)
	{
		LastFrameCounter = CurrentFrameCounter;
	}

	const double DeltaSeconds = std::clamp(
		static_cast<double>(CurrentFrameCounter - LastFrameCounter) * GetSecondsPerCycle(),
		0.0,
		0.25);

	LastFrameCounter = CurrentFrameCounter;
	FrameTimeMs = DeltaSeconds * 1000.0;
	FramesPerSecond = DeltaSeconds > 1.e-9 ? (1.0 / DeltaSeconds) : 0.0;
}

void FStatsSystem::EndFrame()
{
	++FrameNumber;
}

void FStatsSystem::ApplyPickState(const FPickState& InPickState)
{
	LastPickTimeMs = InPickState.LastPickTimeMs;
	TotalPickTimeMs = InPickState.TotalPickTimeMs;
	TotalPickCount = InPickState.TotalPickCount;
}
