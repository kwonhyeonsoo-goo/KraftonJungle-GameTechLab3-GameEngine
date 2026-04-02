#pragma once
#include "EngineAPI.h"
#include "Math/Point.h"

struct ENGINE_API FRect
{
public:
	FRect() = default;
	FRect(float InX, float InY, float InWidth, float InHeight)
		: Position(InX, InY), Size(InWidth, InHeight) {}
	FRect(const FPoint& InPosition, const FPoint& InSize)
		: Position(InPosition), Size(InSize) {}

	bool ContainsPoint(const FPoint& Point) const;

	FPoint Position;
	FPoint Size;
};
