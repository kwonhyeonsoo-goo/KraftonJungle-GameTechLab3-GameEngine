#pragma once

#include "Types/Array.h"
#include "Types/PlatformTypes.h"
#include "Math/Vector.h"

class FCamera;
class FScene;

struct FPlane
{
	FVector Normal = FVector::ForwardVector;
	float Distance = 0.0f;

	float GetSignedDistanceToPoint(const FVector& Point) const
	{
		return FVector::DotProduct(Normal, Point) + Distance;
	}
};

struct FFrustum
{
	FPlane Planes[6];
};

struct FVisibilityResults
{
	uint64 FrameNumber = 0;
	TArray<uint32> VisiblePrimitiveIndices;
};

class FVisibilitySystem
{
public:
	void Reset();
	void Build(const FScene& InScene, const FCamera& InCamera, FVisibilityResults& OutResults);

private:
	FFrustum BuildFrustum(const FCamera& InCamera) const;
	bool IntersectsAABB(const FFrustum& InFrustum, const FVector& InBoxMin, const FVector& InBoxMax) const;

private:
	uint64 NextFrameNumber = 1;
};
