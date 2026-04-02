#pragma once
#include "EngineAPI.h"

struct ENGINE_API FPoint
{
public:
	FPoint() = default;
	FPoint(float InX, float InY) : X(InX), Y(InY) {}

	FPoint operator+(const FPoint& Other) const { return FPoint(X + Other.X, Y + Other.Y); }
	FPoint operator-(const FPoint& Other) const { return FPoint(X - Other.X, Y - Other.Y); }

	float X = 0.0f;
	float Y = 0.0f;
};