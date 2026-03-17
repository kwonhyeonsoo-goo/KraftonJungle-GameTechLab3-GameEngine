#include "FTimer.h"

void FTimer::Initialize()
{
	LastTime = Clock::now();
}

void FTimer::Tick()
{
	if (TargetFrameTime > 0.0f)
	{
		Clock::time_point Now;
		float Elapsed;
		do
		{
			Now = Clock::now();
			Elapsed = std::chrono::duration<float>(Now - LastTime).count();
		} while (Elapsed < TargetFrameTime);
	}

	Clock::time_point CurrentTime = Clock::now();
	DeltaTime = std::chrono::duration<float>(CurrentTime - LastTime).count();
	TotalTime += DeltaTime;
	LastTime = CurrentTime;
}

void FTimer::SetMaxFPS(float InMaxFPS)
{
	MaxFPS = InMaxFPS;
	TargetFrameTime = MaxFPS > 0.0f ? 1.0f / MaxFPS : 0.0f;
}
