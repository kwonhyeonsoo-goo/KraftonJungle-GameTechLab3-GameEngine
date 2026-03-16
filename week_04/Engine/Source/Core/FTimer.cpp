#include "FTimer.h"

void FTimer::Initialize()
{
	LastTime = Clock::now();
}

void FTimer::Tick()
{
	Clock::time_point CurrentTime = Clock::now();
	DeltaTime = std::chrono::duration<float>(CurrentTime - LastTime).count();
	TotalTime += DeltaTime;
	LastTime = CurrentTime;
}
