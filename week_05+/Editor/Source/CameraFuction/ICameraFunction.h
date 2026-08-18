#pragma once
#include "ICameraFunction.h"
#include "CoreMinimal.h"
#include "Math/MathUtility.h"

class FCamera;
class ICameraFunction
{
protected:
	FCamera* Camera = nullptr;
	const float MinTransitionTime = 0.01f;
	const float MinOrbitDistance = 0.01f;
	const float MinOrthoWidth = .5f;
	const float MinPerspectiveFOV = 1.f;


	FVector LerpVector(const FVector& A, const FVector& B, float Alpha);
	float LerpFloat(float A, float B, float Alpha);
	float ComputeFocusDistance(float Radius, float FieldOfViewDegrees);
	float LerpAngleDegrees(float Start, float End, float Alpha) const;
	float EvaluateEaseInOut(float T) const;



public:
	virtual bool IsFinished() const = 0;
	void SetCamera(FCamera* InCamera) { Camera = InCamera; }
	virtual void Tick(float DeltaTime) = 0;
};

