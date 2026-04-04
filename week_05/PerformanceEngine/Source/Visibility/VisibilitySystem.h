#pragma once

#include "Types/Array.h"
#include "Types/PlatformTypes.h"
#include "Math/Vector.h"
#include "Scene/BVH/BVH.h"

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
	uint64 NextFrameNumber = 1;
};
