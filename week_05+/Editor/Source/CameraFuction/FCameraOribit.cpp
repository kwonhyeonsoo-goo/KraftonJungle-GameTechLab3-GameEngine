#include "FCameraOribit.h"
#include "Camera/Camera.h"
#include "Math/MathUtility.h"

namespace
{
	constexpr float MinTransitionTime = 0.01f;
	constexpr float MinOrbitDistance = 0.01f;

	FVector LerpVector(const FVector& A, const FVector& B, float Alpha)
	{
		return A + (B - A) * Alpha;
	}

	float LerpFloat(float A, float B, float Alpha)
	{
		return A + (B - A) * Alpha;
	}

	float LerpAngleDegrees(float Start, float End, float Alpha)
	{
		float Delta = std::fmod(End - Start, 360.0f);

		if (Delta > 180.0f)
		{
			Delta -= 360.0f;
		}
		else if (Delta < -180.0f)
		{
			Delta += 360.0f;
		}

		return Start + Delta * Alpha;
	}
}

void FCameraOribit::StartTransition(
	const FVector& InPivotPosition,
	const FVector& InTargetRotation,
	float InTransitionTime)
{
	if (!Camera)
	{
		return;
	}


	// 시작 상태 저장
	StartPosition = Camera->GetPosition();
	StartPosition = Camera->GetPosition();
	StartRotation = FVector(Camera->GetYaw(), Camera->GetPitch(), 0.0f);

	// 목표 상태 설정
	PivotPosition = InPivotPosition;
	TargetRotation = InTargetRotation;

	// Pivot 기준 공전 거리 계산
	OrbitDistance = FMath::Max(FVector::Dist(StartPosition, PivotPosition), MinOrbitDistance);


	// 목표 회전 기준 최종 위치 계산
	TargetPosition = CalculateOrbitPosition(PivotPosition, TargetRotation, OrbitDistance);

	// 시간 초기화
	TransitionTime = FMath::Max(InTransitionTime, MinTransitionTime);
	TransitionTime = FMath::Max(InTransitionTime, MinTransitionTime);
	ElapsedTime = 0.0f;
	bIsTransitioning = true;
}

void FCameraOribit::Tick(float DeltaTime)
{
	UpdateTransition(DeltaTime);
}

bool FCameraOribit::IsFinished() const
{
	return !bIsTransitioning;
}

void FCameraOribit::UpdateTransition(float DeltaTime)
{
	if (!Camera || !bIsTransitioning)
	{
		return;
	}

	ElapsedTime += DeltaTime;

	const float Alpha = FMath::Clamp(ElapsedTime / TransitionTime, 0.0f, 1.0f);
	const float EasedAlpha = EvaluateEaseInOut(Alpha);

	// 회전 보간
	const FVector CurrentRotation = InterpolateRotation(StartRotation, TargetRotation, EasedAlpha);
	Camera->SetRotation(CurrentRotation.X, CurrentRotation.Y);

	// 위치 보간
	FVector CurrentPosition = FVector::ZeroVector;
	CurrentPosition = CalculateOrbitPosition(PivotPosition, CurrentRotation, OrbitDistance);


	Camera->SetPosition(CurrentPosition);

	// 종료 처리
	if (Alpha >= 1.0f)
	{
		Camera->SetRotation(TargetRotation.X, TargetRotation.Y);

		Camera->SetPosition(CalculateOrbitPosition(PivotPosition, TargetRotation, OrbitDistance));


		bIsTransitioning = false;
	}
}

float FCameraOribit::EvaluateEaseInOut(float T) const
{
	const float ClampedT = FMath::Clamp(T, 0.0f, 1.0f);
	return ClampedT * ClampedT * (3.0f - 2.0f * ClampedT);
}

FVector FCameraOribit::CalculateOrbitPosition(
	const FVector& InPivotPosition,
	const FVector& InRotation,
	float InDistance) const
{
	const float YawRadians = FMath::DegreesToRadians(InRotation.X);
	const float PitchRadians = FMath::DegreesToRadians(InRotation.Y);

	FVector Forward;
	Forward.X = std::cos(PitchRadians) * std::cos(YawRadians);
	Forward.Y = std::cos(PitchRadians) * std::sin(YawRadians);
	Forward.Z = std::sin(PitchRadians);
	Forward = Forward.GetSafeNormal();

	return InPivotPosition - Forward * InDistance;
}

FVector FCameraOribit::InterpolateRotation(
	const FVector& InStartRotation,
	const FVector& InTargetRotation,
	float T) const
{
	return FVector(
		LerpAngleDegrees(InStartRotation.X, InTargetRotation.X, T),
		LerpAngleDegrees(InStartRotation.Y, InTargetRotation.Y, T),
		LerpAngleDegrees(InStartRotation.Z, InTargetRotation.Z, T));
}
