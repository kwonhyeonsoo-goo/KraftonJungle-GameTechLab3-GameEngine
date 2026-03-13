#pragma once
#include "CoreMinimal.h"
#include "Object/Object.h"

class ENGINE_API USceneComponent : public UObject
{
public:
	FTransform RelativeTransform;

	FVector GetRelativeLocation() const { return RelativeTransform.Location; }
	FVector GetRelativeRotation() const { return RelativeTransform.Rotation; }
	FVector GetRelativeScale3D() const { return RelativeTransform.Scale; }

	void SetRelativeLocation(const FVector& InLocation) { RelativeTransform.Location = InLocation; }
	void SetRelativeRotation(const FVector& InRotation) { RelativeTransform.Rotation = InRotation; }
	void SetRelativeScale3D(const FVector& InScale) { RelativeTransform.Scale = InScale; }
};

