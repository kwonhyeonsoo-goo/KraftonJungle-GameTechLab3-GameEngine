#pragma once

#include "CoreMinimal.h"
#include "ICameraFunction.h"

class FCamera;
class AActor;

class FPersToPers : public ICameraFunction
{
	FVector TargetPosition;
	FVector StartPosition;
	float TargetOrthoWidth = 0.0f;
	float StartOrthoWidth = 0.0f;

	float FocusTime = 1.0f;
	float MoveElapsedTime = 0.0f;

	bool bIsMoving = false;

public:
	void FocusOnActor(const AActor* TargetActor);
	virtual void Tick(float DeltaTime) override;
	virtual bool IsFinished() const override { return !bIsMoving; }

private:
	float EvaluateEaseInOut(float T) const;
};
