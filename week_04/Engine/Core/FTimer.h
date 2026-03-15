#pragma once
#include "CoreMinimal.h"
#include "Windows.h"

class ENGINE_API FTimer
{
public:
	FTimer() = default;

	void Initialize();
	float GetDeltaTime();

private:
	LARGE_INTEGER Frequency = {};
	LARGE_INTEGER LastTime = {};
};
