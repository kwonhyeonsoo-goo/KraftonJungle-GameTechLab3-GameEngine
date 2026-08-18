#pragma once

#include "Math/Transform.h"

class FInput;

class FCamera
{
public:
	FCamera() = default;
	~FCamera() = default;

	FCamera(const FCamera&) = default;
	FCamera(FCamera&&) = default;
	FCamera& operator=(const FCamera&) = default;
	FCamera& operator=(FCamera&&) = default;

	FMatrix GetViewMatrix() const;
	FMatrix GetProjectionMatrix() const;

	void SetFOV(float InFov);
	void SetAspectRatio(float InAspectRatio);
	void SetNearClip(float InNearClip);
	void SetFarClip(float InFarClip);
	void SetSpeed(float InSpeed);
	void SetSensitivity(float InSensitivity);

	float GetFOV() const noexcept { return FovDegrees; }
	float GetAspectRatio() const noexcept { return AspectRatio; }
	float GetNearClip() const noexcept { return NearClip; }
	float GetFarClip() const noexcept { return FarClip; }
	float GetSpeed() const noexcept { return MovementSpeed; }
	float GetSensitivity() const noexcept { return LookSensitivity; }

	const FTransform& GetTransform() const noexcept { return Transform; }
	const FVector& GetLocation() const noexcept { return CurrentLocation; }
	const FQuat& GetRotation() const noexcept { return Transform.GetRotation(); }

	void SetTransform(const FTransform& InTransform) noexcept;
	void SetLocation(const FVector& InLocation) noexcept;
	void SetRotation(const FQuat& InRotation) noexcept;
	void SetRotation(const FRotator& InRotation) noexcept;

	void Update(const FInput& Input, float DeltaTime);

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value);
	void Rotate(float DeltaYaw, float DeltaPitch);
	void ApplyMovementInput(bool bMoveForward, bool bMoveBackward, bool bMoveRight, bool bMoveLeft, bool bMoveUp, bool bMoveDown, float DeltaTime);
	void CacheViewAnglesFromRotation(const FQuat& InRotation) noexcept;
	void SyncTransformFromState() noexcept;

private:
	FTransform Transform;
	FVector CurrentLocation = FVector::ZeroVector;
	float FovDegrees = 60.0f;
	float AspectRatio = 16.0f / 9.0f;
	float NearClip = 0.1f;
	float FarClip = 1000.0f;
	float MovementSpeed = 600.0f;
	float LookSensitivity = 0.1f;
	float CurrentPitchDegrees = 0.0f;
	float CurrentYawDegrees = 0.0f;
};
