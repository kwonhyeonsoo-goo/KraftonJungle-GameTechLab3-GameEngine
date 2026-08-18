#pragma once

#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Actor/Actor.h"
#include "Math/Ray.h"

class FCamera;

class FPicker
{
public:
	FRay ScreenToRay(const FCamera* Camera, int32 ScreenX, int32 ScreenY, int32 ScreenWidth, int32 ScreenHeight) const;
	bool RayTriangleIntersect(const FRay& Ray, const FVector& V0, const FVector& V1, const FVector& V2, float& OutDistance) const;
	bool RayAABBIntersect(const FRay& Ray, const FVector& BoxMin, const FVector& BoxMax, float& OutDistance) const;
	AActor* PickActor(const TArray<AActor*>& InActors, const FCamera* InCamera, int32 ScreenX, int32 ScreenY, int32 ScreenWidth, int32 ScreenHeight) const;
};
