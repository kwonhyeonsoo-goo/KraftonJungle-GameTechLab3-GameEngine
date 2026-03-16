#pragma once
#include "CoreMinimal.h"
#include <chrono>

class ENGINE_API FTimer
{
public:
	FTimer() = default;

	void Initialize();
	void Tick();
	float GetDeltaTime() const { return DeltaTime; }
	double GetTotalTime() const { return TotalTime; }

private:
	using Clock = std::chrono::high_resolution_clock;
	Clock::time_point LastTime = {};
	float DeltaTime = 0.0f;
	double TotalTime = 0.0;
};
