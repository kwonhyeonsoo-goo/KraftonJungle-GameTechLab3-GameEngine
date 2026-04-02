#pragma once

#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Actor/Actor.h"

class FCamera;

struct FRay
{
	FVector Origin;
	FVector Direction;
};

class FPicker
{
public:
	FRay ScreenToRay(const FCamera* Camera, int32 ScreenX, int32 ScreenY, int32 ScreenWidth, int32 ScreenHeight) const;
	bool RayTriangleIntersect(const FRay& Ray, const FVector& V0, const FVector& V1, const FVector& V2, float& OutDistance) const;
	AActor* PickActor(const TArray<AActor*>& InActors, const FCamera* InCamera, int32 ScreenX, int32 ScreenY, int32 ScreenWidth, int32 ScreenHeight) const;
};
