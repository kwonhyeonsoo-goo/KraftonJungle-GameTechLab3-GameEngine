#pragma once

#include "CoreMinimal.h"

struct ENGINE_API FVector2
{
	float X;
	float Y;

	FVector2()
		: X(0.0f), Y(0.0f)
	{
	}

	FVector2(float InX, float InY)
		: X(InX), Y(InY)
	{
	}
};

struct ENGINE_API FFontVertex
{
	FVector Position;
	FVector2 UV;

	FFontVertex()
		: Position(), UV()
	{
	}

	FFontVertex(const FVector& InPosition, const FVector2& InUV)
		: Position(InPosition), UV(InUV)
	{
	}
};