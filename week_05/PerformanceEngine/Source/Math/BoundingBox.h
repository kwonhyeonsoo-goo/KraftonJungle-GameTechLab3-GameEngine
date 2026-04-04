#pragma once

#include "Math/Vector.h"
#include <algorithm>

struct FBoundingBox
{
	FVector Min;
	FVector Max;

	void Encapsulate(const FBoundingBox& Other);

	inline FVector GetCenter() const { return (Min + Max) * 0.5f; }
	inline FVector GetExtents() const { return (Max - Min) * 0.5f; }

	inline float GetLongestAxis() const
	{
		FVector Extents = GetExtents();
		return std::max(Extents.X, std::max(Extents.Y, Extents.Z));
	}
};
