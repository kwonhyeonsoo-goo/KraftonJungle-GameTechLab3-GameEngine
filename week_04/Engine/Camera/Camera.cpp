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
	return Forward.GetSafeNormal();
}

FVector CCamera::GetRight() const
{
	return FVector::CrossProduct(Up, GetForward()).GetSafeNormal();
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
	return FMatrix::MakeViewLookAtLH(Position, Target, Up);
}

FMatrix CCamera::GetProjectionMatrix() const
{
	constexpr float DegreesToRadians = 3.14159265358979f / 180.0f;
	return FMatrix::MakePerspectiveFovLH(FOV * DegreesToRadians, AspectRatio, NearPlane, FarPlane);
}

void CCamera::SetAspectRatio(float InAspectRatio)
{
	AspectRatio = InAspectRatio;
}

FVector CCamera::GetPosition() const
{
	return Position;
}

float CCamera::GetYaw() const
{
	return Yaw;
}

float CCamera::GetPitch() const
{
	return Pitch;
}

float CCamera::GetFOV() const
{
	return FOV;
}

void CCamera::SetFOV(float InFOV)
{
	FOV = InFOV;
}
