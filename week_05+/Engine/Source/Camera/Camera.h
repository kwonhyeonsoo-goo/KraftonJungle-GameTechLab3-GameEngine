#pragma once

#include "CoreMinimal.h"

enum class ECameraProjectionMode : uint8_t
{
	Perspective,
	Orthographic
};

class ENGINE_API FCamera
{
public:
	FCamera() = default;

	void SetPosition(const FVector& InPosition);
	void SetRotation(float InYaw, float InPitch);
	void SetOrbitTarget(const FVector& InOrbitTarget); //orbit rotation을 하는 뷰어에서만 사용

	void MoveForward(float Delta);
	void MoveRight(float Delta);
	void MoveUp(float Delta);
	void Rotate(float DeltaYaw, float DeltaPitch);
	void OffsetPosition(const FVector& Delta);

	FVector GetForward() const;
	FVector GetRight() const;

	FMatrix GetViewMatrix() const;
	FMatrix GetProjectionMatrix() const;

	void SetAspectRatio(float InAspectRatio);
	void SetSpeed(float InSpeed) { Speed = InSpeed; }
	float GetSpeed() const { return Speed; }
	float GetMouseSensitivity() const { return Sensitivity; }
	void SetMouseSensitivity(float InSensitivity) { Sensitivity = InSensitivity; }
	FVector GetPosition() const;
	float GetYaw() const;
	float GetPitch() const;
	float GetFOV() const;
	void SetFOV(float InFOV);
	float GetNearClip() const;
	float GetFarClip() const;
	void SetNearClip(float InNearClip);
	void SetFarClip(float InFarClip);
	ECameraProjectionMode GetProjectionMode() const;
	bool IsOrthographic() const;
	void SetProjectionMode(ECameraProjectionMode InProjectionMode);
	float GetOrthoWidth() const;
	float GetOrthoHeight() const;
	void SetOrthoWidth(float InOrthoWidth);
	void SetOrthoHeight(float InOrthoHeight);
	FVector GetOrbitTarget() const { return OrbitTarget; }

private:
	FVector Position = { -3.0f, -3.0f, 2.5f };
	FVector Up = { 0.0f, 0.0f, 1.0f };

	float Yaw = 45.0f;
	float Pitch = -30.0f;
	float Speed = 5.0f;
	float Sensitivity = 0.2f;
	float FOV = 45.0f;
	float AspectRatio = 16.0f / 9.0f;
	ECameraProjectionMode ProjectionMode = ECameraProjectionMode::Perspective;
	float OrthoWidth = 20.0f;
	float NearPlane = 0.1f;
	float FarPlane = 10000.0f;
	float OrbitDistance = 0.0f;
	FVector OrbitTarget = FVector::ZeroVector;
};
