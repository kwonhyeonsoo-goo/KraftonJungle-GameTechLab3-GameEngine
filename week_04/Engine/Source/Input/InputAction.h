#pragma once

#include "CoreMinimal.h"
#include <cmath>

enum class EInputKey : int32
{
	MouseX = 256,
	MouseY = 257,
};

enum class EInputActionValueType : uint32
{
	Bool,
	Float,
	Axis2D,
	Axis3D,
};

struct ENGINE_API FInputActionValue
{

};