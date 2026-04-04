#pragma once
#include "Types/PlatformTypes.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

class FScopeCycleCounter
{
public:
	FScopeCycleCounter()
	{
		LARGE_INTEGER Counter = {};
		QueryPerformanceCounter(&Counter);
		StartCycles = static_cast<uint64>(Counter.QuadPart);
	}

	double Finish() const
	{
		LARGE_INTEGER Counter = {};
		QueryPerformanceCounter(&Counter);
		uint64 EndCycles = static_cast<uint64>(Counter.QuadPart);

		return CyclesToMilliseconds(StartCycles, EndCycles);
	}

private:
	uint64 StartCycles;

	static double CyclesToMilliseconds(uint64 InStartCycles, uint64 InEndCycles)
	{
		static const double SecondsPerCycle = []()
		{
			LARGE_INTEGER Frequency = {};
			QueryPerformanceFrequency(&Frequency);
			return 1.0 / static_cast<double>(Frequency.QuadPart);
		}();
		return static_cast<double>(InEndCycles - InStartCycles) * SecondsPerCycle * 1000.0;
	}
};