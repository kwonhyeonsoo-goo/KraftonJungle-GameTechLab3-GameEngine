#pragma once

#include "EngineAPI.h"
#include "Math/Vector.h"

struct ENGINE_API FRay
{
	FVector Origin;
	FVector Direction;
	FVector InvDirection;
};