#pragma once
#include "../CoreMinimal.h"

struct FCameraViewInfo
{
	FVector Position = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector;
	FVector Right = FVector::RightVector;
	FVector Up = FVector::UpVector;

	FMatrix ViewMatrix = FMatrix::Identity;
	FMatrix ProjectionMatrix = FMatrix::Identity;

	float FOV = 45.0f;
	float OrthoWidth = 20.0f;
	float OrthoHeight = 20.0f;
	float AspectRatio = 16.0f / 9.0f;
	bool  bIsOrthographic = false;
};