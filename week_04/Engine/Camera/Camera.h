#pragma once

#include "CoreMinimal.h"

class ENGINE_API CCamera
{
public:
	CCamera() = default;

	void SetPosition(const FVector& InPosition);
	void SetRotation(float InYaw, float InPitch);

	void MoveForward(float Delta);
	void MoveRight(float Delta);
	void MoveUp(float Delta);
	void Rotate(float DeltaYaw, float DeltaPitch);

	FVector GetForward() const;
	FVector GetRight() const;

	FMatrix GetViewMatrix() const;
	FMatrix GetProjectionMatrix() const;

	void SetAspectRatio(float InAspectRatio);
	void SetSpeed(float InSpeed) { Speed = InSpeed; }

	FVector GetPosition() const;
	float GetYaw() const;
	float GetPitch() const;
	float GetFOV() const;
	void SetFOV(float InFOV);

private:
	FVector Position = { -5.0f, 0.0f, 2.0f };
	FVector Up = { 0.0f, 0.0f, 1.0f };

	float Yaw = 0.0f;
	float Pitch = 0.0f;
	float Speed = 5.0f;

	float FOV = 45.0f;
	float AspectRatio = 16.0f / 9.0f;
	float NearPlane = 0.1f;
	float FarPlane = 1000.0f;
};
