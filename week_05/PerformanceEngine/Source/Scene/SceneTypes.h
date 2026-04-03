#pragma once

#include <limits>
#include <memory>

#include "Math/Transform.h"
#include "Types/PlatformTypes.h"
#include "Types/String.h"

class FStaticMesh;

struct FRay
{
	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
};

struct FPickHit
{
	int32 PrimitiveId = -1;
	int32 PrimitiveIndex = -1;
	float DistanceSquared = std::numeric_limits<float>::max();
	FVector WorldPosition = FVector::ZeroVector;
};

struct FSceneCameraInitData
{
	FTransform Transform = FTransform::Identity;
	float FovDegrees = 60.0f;
	float NearClip = 0.1f;
	float FarClip = 1000.0f;
};

struct FRenderItem
{
	int32 PrimitiveId = -1;
	FString MeshAssetPath;
	FTransform Transform = FTransform::Identity;
	FVector WorldBoundsMin = FVector::ZeroVector;
	FVector WorldBoundsMax = FVector::ZeroVector;
	std::shared_ptr<FStaticMesh> StaticMesh;
};
