#include "FPerspectToOrtho.h"

#include "Actor/Actor.h"
#include "Camera/Camera.h"
#include "Math/MathUtility.h"
#include "Component/PrimitiveComponent.h"

#include <cmath>

namespace
{
	float ComputePerspectiveHalfWidth(float Distance, float FieldOfViewDegrees, float AspectRatio)
	{
		const float SafeDistance = FMath::Max(Distance, 0.01f);
		const float SafeAspectRatio = FMath::Max(AspectRatio, 0.01f);
		const float HalfFovRadians = FMath::DegreesToRadians(FMath::Max(FieldOfViewDegrees, 0.1f) * 0.5f);
		const float SafeTanHalfFov = FMath::Max(std::tanf(HalfFovRadians), 0.01f);
		return SafeDistance * SafeTanHalfFov * SafeAspectRatio;
	}

	float ComputeDistanceForHalfWidth(float HalfWidth, float FieldOfViewDegrees, float AspectRatio)
	{
		const float SafeHalfWidth = FMath::Max(HalfWidth, 0.01f);
		const float SafeAspectRatio = FMath::Max(AspectRatio, 0.01f);
		const float HalfFovRadians = FMath::DegreesToRadians(FMath::Max(FieldOfViewDegrees, 0.1f) * 0.5f);
		const float SafeTanHalfFov = FMath::Max(std::tanf(HalfFovRadians), 0.01f);
		return SafeHalfWidth / (SafeTanHalfFov * SafeAspectRatio);
	}
}

void FPerspectToOrtho::StartTransition(const FVector& InPivotPosition, float InTransitionTime)
{
	if (!Camera)
	{
		return;
	}

	StartFOV = FMath::Max(Camera->GetFOV(), MinPerspectiveFOV);
	ViewDirection = Camera->GetForward().GetSafeNormal();
	if (ViewDirection.IsNearlyZero())
	{
		ViewDirection = FVector::ForwardVector;
	}

	StartPosition = Camera->GetPosition();
	const float FocusDistance = FMath::Max(
		FVector::DotProduct(InPivotPosition - StartPosition, ViewDirection),
		MinOrbitDistance);
	PivotPosition = StartPosition + ViewDirection * FocusDistance;
	ViewAspectRatio = FMath::Max(
		Camera->GetOrthoWidth() / FMath::Max(Camera->GetOrthoHeight(), 0.01f),
		0.01f);

	const float StartHalfWidth = FMath::Max(
		ComputePerspectiveHalfWidth(FocusDistance, StartFOV, ViewAspectRatio),
		MinOrthoWidth * 0.5f);
	TargetOrthoWidth = StartHalfWidth * 2.0f;
	TargetPosition = PivotPosition - ViewDirection * ComputeDistanceForHalfWidth(StartHalfWidth, MinPerspectiveFOV, ViewAspectRatio);

	FocusTime = FMath::Max(InTransitionTime, MinTransitionTime);
	MoveElapsedTime = 0.0f;
	bIsTransition = true;
}

void FPerspectToOrtho::Tick(float DeltaTime)
{
	if (!Camera || !bIsTransition)
	{
		return;
	}

	MoveElapsedTime += DeltaTime;
	const float MoveAlpha = FocusTime > 0.0f ? FMath::Clamp(MoveElapsedTime / FocusTime, 0.0f, 1.0f) : 1.0f;
	const float EasedAlpha = EvaluateEaseInOut(MoveAlpha);

	const float CurrentFov = LerpFloat(StartFOV, MinPerspectiveFOV, EasedAlpha);
	const float CurrentHalfWidth = TargetOrthoWidth * 0.5f;
	const float CurrentDistance = ComputeDistanceForHalfWidth(CurrentHalfWidth, CurrentFov, ViewAspectRatio);
	const FVector CurrentPosition = PivotPosition - ViewDirection * CurrentDistance;

	Camera->SetFOV(CurrentFov);
	Camera->SetPosition(CurrentPosition);


	if (MoveAlpha >= 1.0f)
	{
		bIsTransition = false;
		Camera->SetPosition(TargetPosition);
		Camera->SetFOV(MinPerspectiveFOV);
		Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
		Camera->SetOrthoWidth(TargetOrthoWidth);
	}
}
