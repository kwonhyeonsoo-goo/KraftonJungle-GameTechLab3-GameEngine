
#pragma once
#include "SceneComponent.h"
#include "../CoreMinimal.h"
#include "Camera/CameraInfo.h"


class ENGINE_API UCameraComponent : public USceneComponent
{
public:
	DECLARE_RTTI(UCameraComponent, USceneComponent)

	virtual void Tick(float DeltaTime) override;

	// ── 이동 / 회전 ──────────────────────────────────────
	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value);
	void PanRight(float Value);
	void PanUp(float Value);
	void Rotate(float DeltaYaw, float DeltaPitch);

	// ── 행렬 ─────────────────────────────────────────────
	FMatrix GetViewMatrix() const;
	FMatrix GetProjectionMatrix() const;

	// ── ViewInfo 스냅샷 (외부 시스템이 사용하는 유일한 창구) ──
	FCameraViewInfo GetViewInfo() const;

	// ── Getter ────────────────────────────────────────────
	FVector GetPosition()  const { return Position; }
	FVector GetForward()   const;
	FVector GetRight()     const;
	float   GetYaw()       const { return Yaw; }
	float   GetPitch()     const { return Pitch; }
	float   GetFOV()       const { return FOV; }
	float   GetAspectRatio() const { return AspectRatio; }
	float   GetOrthoWidth()  const { return OrthoWidth; }
	float   GetOrthoHeight() const { return OrthoWidth / FMath::Max(AspectRatio, 0.01f); }
	bool    IsOrthographic() const { return bIsOrthographic; }
	float   GetSpeed()        const { return Speed; }
	float   GetSensitivity()  const { return Sensitivity; }

	// ── Setter ────────────────────────────────────────────
	void SetPosition(const FVector& InPosition) { Position = InPosition; }
	void SetRotation(float InYaw, float InPitch);
	void SetFOV(float InFOV);
	void SetAspectRatio(float InAspectRatio);
	void SetOrthoWidth(float InOrthoWidth);
	void SetOrthographic(bool bInOrthographic) { bIsOrthographic = bInOrthographic; }
	void SetSpeed(float InSpeed) { Speed = InSpeed; }
	void SetSensitivity(float InSensitivity) { Sensitivity = InSensitivity; }

private:
	// ── Transform ─────────────────────────────────────────
	FVector Position = { -5.0f, 0.0f, 2.0f };
	FVector Up = { 0.0f, 0.0f, 1.0f };
	float   Yaw = 0.0f;
	float   Pitch = 0.0f;

	// ── 렌즈 파라미터 ──────────────────────────────────────
	float FOV = 45.0f;
	float AspectRatio = 16.0f / 9.0f;
	float NearPlane = 0.1f;
	float FarPlane = 1000.0f;
	float OrthoWidth = 20.0f;
	bool  bIsOrthographic = false;

	// ── 컨트롤 파라미터 (추후 컨트롤러 레이어로 분리 예정) ──
	float Speed = 5.0f;
	float Sensitivity = 0.2f;
};