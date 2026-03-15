#include "FTimer.h"

void FTimer::Initialize()
{
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&LastTime);
}

float FTimer::GetDeltaTime()
{
	LARGE_INTEGER CurrentTime;
	QueryPerformanceCounter(&CurrentTime);

	float DeltaTime = static_cast<float>(CurrentTime.QuadPart - LastTime.QuadPart)
		/ static_cast<float>(Frequency.QuadPart);
	LastTime = CurrentTime;

	return DeltaTime;
}
