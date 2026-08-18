#pragma once
#include "ICameraFunction.h"
#include "CoreMinimal.h"



class AActor;
class FPerspectToOrtho : public ICameraFunction
{
private:
	float StartFOV  = 45.f;
	float TargetOrthoWidth = 20.f;
	float ViewAspectRatio = 16.0f / 9.0f;
	FVector StartPosition = FVector::ZeroVector;
	FVector TargetPosition = FVector::ZeroVector;
	FVector ViewDirection = FVector::ZeroVector;
	FVector PivotPosition = FVector::ZeroVector;
	
	bool bIsTransition = false;
	float MoveElapsedTime = 0.0f;
	float FocusTime = 1.f;

public:
	void StartTransition(const FVector& InPivotPosition, float InTransitionTime);
	virtual bool IsFinished() const override { return !bIsTransition; }
	virtual void Tick(float DeltaTime) override;
};
