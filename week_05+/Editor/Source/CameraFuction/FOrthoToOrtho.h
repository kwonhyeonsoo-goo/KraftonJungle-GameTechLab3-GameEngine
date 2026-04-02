#pragma once

#include "CoreMinimal.h"
#include "ICameraFunction.h"

class FCamera;

/**
 * @brief Orthographic 카메라 시점을 다른 Orthographic 시점으로 부드럽게 전환하는 클래스
 *
 * 예:
 * - Top View -> Front View
 * - Front View -> Right View
 *
 * PivotPosition을 기준으로 카메라가 공전하듯 이동하면서
 * 회전이 바뀌도록 하여 "카메라가 돌아가는 느낌"을 만든다.
 */
class FOrthoToOrtho : public ICameraFunction
{
private:
	// Rotation 벡터의 의미:
	// X = Yaw, Y = Pitch, Z = Roll

	// ===== 시작 상태 =====

	// 전환 시작 시 카메라 위치
	FVector StartPosition = FVector::ZeroVector;

	// 전환 시작 시 카메라 회전
	FVector StartRotation = FVector::ZeroVector;

	// 전환 시작 시 Ortho Width
	float StartOrthoWidth = 0.0f;


	// ===== 목표 상태 =====

	// 전환 종료 시 카메라 위치
	// bUseOrbitMovement == true 인 경우, TargetRotation 기준 orbit 위치를 캐시한 값으로 사용한다.
	FVector TargetPosition = FVector::ZeroVector;

	// 전환 종료 시 카메라 회전
	FVector TargetRotation = FVector::ZeroVector;

	// 전환 종료 시 Ortho Width
	float TargetOrthoWidth = 0.0f;


	// ===== 회전 기준 =====

	// 카메라가 회전할 중심점
	FVector PivotPosition = FVector::ZeroVector;

	// Pivot으로부터 카메라까지의 거리
	float OrbitDistance = 0.0f;


	// ===== 시간 제어 =====

	// 전체 전환 시간
	float TransitionTime = 1.0f;

	// 현재까지 경과 시간
	float ElapsedTime = 0.0f;

	// 현재 전환 진행 여부
	bool bIsTransitioning = false;


	// ===== 옵션 =====

	// true면 Pivot 기준 공전 이동, false면 StartPosition -> TargetPosition 선형 보간
	bool bUseOrbitMovement = true;

	// true면 Ortho Width도 함께 보간
	bool bChangeOrthoWidth = false;


public:
	/**
	 * @brief Ortho -> Ortho 시점 전환 시작
	 *
	 * @param InPivotPosition   회전 기준점
	 * @param InTargetRotation  목표 회전값 (X=Yaw, Y=Pitch, Z=Roll)
	 * @param InTransitionTime  전환 시간
	 */
	void StartTransition(
		const FVector& InPivotPosition,
		const FVector& InTargetRotation,
		float InTransitionTime);

	/**
	 * @brief 매 프레임 호출되어 전환을 진행한다.
	 */
	void Tick(float DeltaTime);

	/**
	 * @brief 전환 완료 여부 반환
	 */
	bool IsFinished() const;


private:
	/**
	 * @brief 실제 전환 업데이트 로직
	 */
	void UpdateTransition(float DeltaTime);

	/**
	 * @brief 0~1 범위 EaseInOut 함수
	 */
	float EvaluateEaseInOut(float T) const;

	/**
	 * @brief Pivot, 회전, 거리로부터 orbit 위치를 계산한다.
	 */
	FVector CalculateOrbitPosition(
		const FVector& InPivotPosition,
		const FVector& InRotation,
		float InDistance) const;

	/**
	 * @brief 회전값을 각도 wrapping을 고려하여 보간한다.
	 */
	FVector InterpolateRotation(
		const FVector& InStartRotation,
		const FVector& InTargetRotation,
		float T) const;
};