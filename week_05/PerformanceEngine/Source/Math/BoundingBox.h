#pragma once

#include "Math/Vector.h"
#include <algorithm>

struct FRay;

struct FBoundingBox
{
	FVector Min;
	FVector Max;

	void Encapsulate(const FBoundingBox& Other);
	void Encapsulate(const FVector& Other);

	inline FVector GetCenter() const { return (Min + Max) * 0.5f; }
	inline FVector GetExtents() const { return (Max - Min) * 0.5f; }

	inline int32 GetLongestAxis() const
	{
		FVector Extents = GetExtents();
		if (Extents.X >= Extents.Y && Extents.X >= Extents.Z)
		{
			return 0;
		}
		else if (Extents.Y >= Extents.X && Extents.Y >= Extents.Z)
		{
			return 1;
		}
		else
		{
			return 2;
		}
	}
	bool IntersectsRay(const FRay& Ray) const;

	bool Contains(const FVector& Center) const;

	float GetSurfaceArea() const
	{
		FVector Extents = GetExtents();
		return 2.0f * (Extents.X * Extents.Y + Extents.Y * Extents.Z + Extents.Z * Extents.X);
	}
};
