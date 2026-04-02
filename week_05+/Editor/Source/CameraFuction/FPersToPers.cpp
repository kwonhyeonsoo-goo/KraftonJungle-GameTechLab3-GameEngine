#include "FPersToPers.h"

#include "Actor/Actor.h"
#include "Camera/Camera.h"
#include "Component/PrimitiveComponent.h"
#include "Math/MathUtility.h"

#include <cmath>

namespace
{
	FVector LerpVector(const FVector& A, const FVector& B, float Alpha)
	{
		return A + (B - A) * Alpha;
	}

	float LerpFloat(float A, float B, float Alpha)
	{
		return A + (B - A) * Alpha;
	}

	float ComputeFocusDistance(float Radius, float FieldOfViewDegrees)
	{
		const float SafeRadius = FMath::Max(Radius, 0.5f);
		const float HalfFovRadians = FMath::DegreesToRadians(FieldOfViewDegrees * 0.5f);
		const float SafeTanHalfFov = FMath::Max(std::tanf(HalfFovRadians), 0.01f);
		return FMath::Max((SafeRadius / SafeTanHalfFov) * 1.2f, SafeRadius * 2.0f);
	}
}

void FPersToPers::FocusOnActor(const AActor* TargetActor)
{
	if (!Camera || !TargetActor)
	{
		return;
	}

	UPrimitiveComponent* PrimitiveComponent = TargetActor->GetComponentByClass<UPrimitiveComponent>();
	if (!PrimitiveComponent)
	{
		return;
	}

	const FBoxSphereBounds Bounds = PrimitiveComponent->GetWorldBounds();
	StartPosition = Camera->GetPosition();
	StartOrthoWidth = Camera->GetOrthoWidth();
	TargetOrthoWidth = StartOrthoWidth;

	FVector ViewDirection = Camera->GetForward().GetSafeNormal();
	if (ViewDirection.IsNearlyZero())
	{
		ViewDirection = FVector::ForwardVector;
	}

	const float FocusDistance = ComputeFocusDistance(Bounds.Radius, Camera->GetFOV());
	TargetPosition = Bounds.Center - ViewDirection * FocusDistance;

	if (Camera->IsOrthographic())
	{
		TargetOrthoWidth = FMath::Max(Bounds.BoxExtent.Size() * 2.4f, 1.0f);
	}

	MoveElapsedTime = 0.0f;
	bIsMoving = true;
}

void FPersToPers::Tick(float DeltaTime)
{
	if (!Camera || !bIsMoving)
	{
		return;
	}

	MoveElapsedTime += DeltaTime;
	const float MoveAlpha = FocusTime > 0.0f ? FMath::Clamp(MoveElapsedTime / FocusTime, 0.0f, 1.0f) : 1.0f;
	const float EasedAlpha = EvaluateEaseInOut(MoveAlpha);
	const FVector CurrentPosition = LerpVector(StartPosition, TargetPosition, EasedAlpha);
	Camera->SetPosition(CurrentPosition);

	if (Camera->IsOrthographic())
	{
		Camera->SetOrthoWidth(LerpFloat(StartOrthoWidth, TargetOrthoWidth, EasedAlpha));
	}

	if (MoveAlpha >= 1.0f)
	{
		bIsMoving = false;
	}
}

float FPersToPers::EvaluateEaseInOut(float T) const
{
	const float ClampedT = FMath::Clamp(T, 0.0f, 1.0f);
	return ClampedT * ClampedT * (3.0f - 2.0f * ClampedT);
}
