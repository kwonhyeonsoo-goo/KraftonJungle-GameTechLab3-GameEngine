#include "FOrthoToPerspect.h"

#include "Camera/Camera.h"
#include "Math/MathUtility.h"

#include "Actor/Actor.h"
#include "Component/PrimitiveComponent.h"

#include <cmath>

namespace
{
	float ComputeDistanceForHalfWidth(float HalfWidth, float FieldOfViewDegrees, float AspectRatio)
	{
		const float SafeHalfWidth = FMath::Max(HalfWidth, 0.01f);
		const float SafeAspectRatio = FMath::Max(AspectRatio, 0.01f);
		const float HalfFovRadians = FMath::DegreesToRadians(FMath::Max(FieldOfViewDegrees, 0.1f) * 0.5f);
		const float SafeTanHalfFov = FMath::Max(std::tanf(HalfFovRadians), 0.01f);
		return SafeHalfWidth / (SafeTanHalfFov * SafeAspectRatio);
	}
}

void FOrthoToPerspect::StartTransition(float InTargetFov, const FVector& InPivotPosition, float InTransitionTime)
{
	if (!Camera)
	{
		return;
	}

	TargetFov = FMath::Max(InTargetFov, MinPerspectiveFOV);
	ViewDirection = Camera->GetForward().GetSafeNormal();
	if (ViewDirection.IsNearlyZero())
	{
		ViewDirection = FVector::ForwardVector;
	}

	const FVector CurrentPosition = Camera->GetPosition();
	const float FocusDistance = FMath::Max(
		FVector::DotProduct(InPivotPosition - CurrentPosition, ViewDirection),
		MinOrbitDistance);
	PivotPosition = CurrentPosition + ViewDirection * FocusDistance;
	StartOrthoWidth = FMath::Max(Camera->GetOrthoWidth(), MinOrthoWidth);
	ViewAspectRatio = FMath::Max(
		Camera->GetOrthoWidth() / FMath::Max(Camera->GetOrthoHeight(), 0.01f),
		0.01f);

	const float StartHalfWidth = StartOrthoWidth * 0.5f;
	StartPosition = PivotPosition - ViewDirection * ComputeDistanceForHalfWidth(StartHalfWidth, MinPerspectiveFOV, ViewAspectRatio);
	TargetPosition = PivotPosition - ViewDirection * ComputeDistanceForHalfWidth(StartHalfWidth, TargetFov, ViewAspectRatio);

	Camera->SetPosition(StartPosition);
	Camera->SetFOV(MinPerspectiveFOV);
	Camera->SetProjectionMode(ECameraProjectionMode::Perspective);

	FocusTime = FMath::Max(InTransitionTime, MinTransitionTime);
	MoveElapsedTime = 0.0f;
	bIsTransition = true;
}

void FOrthoToPerspect::Tick(float DeltaTime)
{
	if (!Camera || !bIsTransition)
	{
		return;
	}
	Camera->SetProjectionMode(ECameraProjectionMode::Perspective);

	MoveElapsedTime += DeltaTime;
	const float MoveAlpha = FocusTime > 0.0f ? FMath::Clamp(MoveElapsedTime / FocusTime, 0.0f, 1.0f) : 1.0f;
	const float EasedAlpha = EvaluateEaseInOut(MoveAlpha);

	const float CurrentFov = LerpFloat(MinPerspectiveFOV, TargetFov, EasedAlpha);
	const float CurrentHalfWidth = StartOrthoWidth * 0.5f;
	const float CurrentDistance = ComputeDistanceForHalfWidth(CurrentHalfWidth, CurrentFov, ViewAspectRatio);
	const FVector CurrentPosition = PivotPosition - ViewDirection * CurrentDistance;
	Camera->SetPosition(CurrentPosition);
	Camera->SetFOV(CurrentFov);


	if (MoveAlpha >= 1.0f)
	{
		Camera->SetPosition(TargetPosition);
		Camera->SetFOV(TargetFov);
		bIsTransition = false;
	}
}
