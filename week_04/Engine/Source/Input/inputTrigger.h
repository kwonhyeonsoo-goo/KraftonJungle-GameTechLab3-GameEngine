#pragma once
#include "InputAction.h"

enum class ETriggerState : uint32
{
	None,
	Ongoing,
	Triggered,
};

enum class ETriggerEvent : uint32
{
	None = 0,
	Started = 1 << 0,
	Ongoing = 1 << 1,
	Triggered = 1 << 2,
	Completed = 1 << 3,
	Canceled = 1 << 4,
};

class ENGINE_API FInputTrigger
{
};

class ENGINE_API FInputRelease
{
};
class ENGINE_API FInputHolding
{
};
