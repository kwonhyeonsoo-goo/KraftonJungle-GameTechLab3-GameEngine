#pragma once

#include "Math/BoundingBox.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"

struct FPlane
{
	FVector Normal = FVector::ForwardVector;
	float Distance = 0.0f;

	float GetDistance(const FVector& Point) const
	{
		return FVector::DotProduct(Normal, Point) + Distance;
	}
};

struct FFrustum
{
	FPlane Planes[6];

	void Update(const FMatrix& ViewProjection);

	inline bool IsOutSide(const FBoundingBox& Box) const
	{
		for (int32 i = 0; i < 6; ++i)
		{
			FVector PositiveVertex = Box.Min;
			if (Planes[i].Normal.X >= 0) PositiveVertex.X = Box.Max.X;
			if (Planes[i].Normal.Y >= 0) PositiveVertex.Y = Box.Max.Y;
			if (Planes[i].Normal.Z >= 0) PositiveVertex.Z = Box.Max.Z;

			if (Planes[i].GetDistance(PositiveVertex) < 0.0f)
			{
				return true;
			}
		}
		return false;
	}
};
