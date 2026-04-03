#pragma once

struct FVector2
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
