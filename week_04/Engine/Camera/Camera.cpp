#include "Camera.h"
#include <cmath>

void CCamera::SetPosition(const FVector& InPosition)
{
	Position = InPosition;
}

void CCamera::SetRotation(float InYaw, float InPitch)
{
	Yaw = InYaw;
	Pitch = InPitch;
}

FVector CCamera::GetForward() const
{
	const float PI = 3.14159265358979f;
	float RadYaw = Yaw * PI / 180.0f;
	float RadPitch = Pitch * PI / 180.0f;

	// 변경 (Z-up, 언리얼 방식)
	FVector Forward;
	Forward.X = cosf(RadPitch) * cosf(RadYaw);   // X가 Forward
	Forward.Y = cosf(RadPitch) * sinf(RadYaw);   // Y가 Right
	Forward.Z = sinf(RadPitch);                   // Z가 상하
	return Forward.Normalize();
}

FVector CCamera::GetRight() const
{
	return Up.Cross(GetForward()).Normalize();
}

void CCamera::MoveForward(float Delta)
{
	FVector Forward = GetForward();
	Position = Position + Forward * (Delta * Speed);
}

void CCamera::MoveRight(float Delta)
{
	FVector Right = GetRight();
	Position = Position + Right * (Delta * Speed);
}

void CCamera::MoveUp(float Delta)
{
	Position = Position + Up * (Delta * Speed);
}

void CCamera::Rotate(float DeltaYaw, float DeltaPitch)
{
	Yaw += DeltaYaw;
	Pitch += DeltaPitch;

	// Pitch 제한 (-89 ~ 89도)
	if (Pitch > 89.0f) Pitch = 89.0f;
	if (Pitch < -89.0f) Pitch = -89.0f;
}

FMatrix CCamera::GetViewMatrix() const
{
	FVector Target = Position + GetForward();
	return FMatrix::LookAt(Position, Target, Up);
}

FMatrix CCamera::GetProjectionMatrix() const
{
	return FMatrix::Perspective(FOV, AspectRatio, NearPlane, FarPlane);
}

void CCamera::SetAspectRatio(float InAspectRatio)
{
	AspectRatio = InAspectRatio;
}
