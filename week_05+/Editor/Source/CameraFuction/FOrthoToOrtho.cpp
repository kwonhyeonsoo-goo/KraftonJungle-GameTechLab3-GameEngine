#include "FOrthoToOrtho.h"

#include "Camera/Camera.h"
#include "Math/MathUtility.h"

#include <cmath>

void FOrthoToOrtho::StartTransition(
	const FVector& InPivotPosition,
	const FVector& InTargetRotation,
	float InTransitionTime)
{
	if (!Camera)
	{
		return;
	}

	// 이 전환은 Ortho -> Ortho 전환이므로 시작 시 Orthographic으로 맞춘다.
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);

	// 시작 상태 저장
	StartPosition = Camera->GetPosition();
	StartPosition = Camera->GetPosition();
	StartRotation = FVector(Camera->GetYaw(), Camera->GetPitch(), 0.0f);
	StartOrthoWidth = Camera->GetOrthoWidth();

	// 목표 상태 설정
	PivotPosition = InPivotPosition;
	TargetRotation = InTargetRotation;
	TargetOrthoWidth = StartOrthoWidth;

	// Pivot 기준 공전 거리 계산
	OrbitDistance = FMath::Max(FVector::Dist(StartPosition, PivotPosition), MinOrbitDistance);

	// 현재 버전은 기본적으로 orbit 이동 사용 / ortho width 변화 없음
	bUseOrbitMovement = true;
	bChangeOrthoWidth = false;

	// 목표 회전 기준 최종 위치 계산
	TargetPosition = CalculateOrbitPosition(PivotPosition, TargetRotation, OrbitDistance);

	// 시간 초기화
	TransitionTime = FMath::Max(InTransitionTime, MinTransitionTime);
	TransitionTime = FMath::Max(InTransitionTime, MinTransitionTime);
	ElapsedTime = 0.0f;
	bIsTransitioning = true;
}

void FOrthoToOrtho::Tick(float DeltaTime)
{
	UpdateTransition(DeltaTime);
}

bool FOrthoToOrtho::IsFinished() const
{
	return !bIsTransitioning;
}

void FOrthoToOrtho::UpdateTransition(float DeltaTime)
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
	if (bUseOrbitMovement)
	{
		// 현재 회전값을 기준으로 Pivot 주위를 공전하는 위치 계산
		CurrentPosition = CalculateOrbitPosition(PivotPosition, CurrentRotation, OrbitDistance);
	}
	else
	{
		// 일반 위치 선형 보간
		CurrentPosition = LerpVector(StartPosition, TargetPosition, EasedAlpha);
	}

	Camera->SetPosition(CurrentPosition);

	// 필요 시 Ortho Width 보간
	if (bChangeOrthoWidth)
	{
		Camera->SetOrthoWidth(LerpFloat(StartOrthoWidth, TargetOrthoWidth, EasedAlpha));
	}

	// 종료 처리
	if (Alpha >= 1.0f)
	{
		Camera->SetRotation(TargetRotation.X, TargetRotation.Y);

		if (bUseOrbitMovement)
		{
			Camera->SetPosition(CalculateOrbitPosition(PivotPosition, TargetRotation, OrbitDistance));
		}
		else
		{
			Camera->SetPosition(TargetPosition);
		}

		if (bChangeOrthoWidth)
		{
			Camera->SetOrthoWidth(TargetOrthoWidth);
		}

		bIsTransitioning = false;
	}
}

float FOrthoToOrtho::EvaluateEaseInOut(float T) const
{
	const float ClampedT = FMath::Clamp(T, 0.0f, 1.0f);
	return ClampedT * ClampedT * (3.0f - 2.0f * ClampedT);
}

FVector FOrthoToOrtho::CalculateOrbitPosition(
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

FVector FOrthoToOrtho::InterpolateRotation(
	const FVector& InStartRotation,
	const FVector& InTargetRotation,
	float T) const
{
	return FVector(
		LerpAngleDegrees(InStartRotation.X, InTargetRotation.X, T),
		LerpAngleDegrees(InStartRotation.Y, InTargetRotation.Y, T),
		LerpAngleDegrees(InStartRotation.Z, InTargetRotation.Z, T));
}
