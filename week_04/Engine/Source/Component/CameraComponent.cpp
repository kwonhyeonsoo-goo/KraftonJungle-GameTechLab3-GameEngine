#include "CameraComponent.h"
#include "Object/Class.h"
#include "Math/MathUtility.h"
#include <algorithm>
#include <cmath>

IMPLEMENT_RTTI(UCameraComponent, USceneComponent)

// ── Tick ───────────────────────────────────────────────────────────────────

void UCameraComponent::Tick(float DeltaTime)
{
	USceneComponent::Tick(DeltaTime);
	// TODO: CameraArm, Shake, Interpolation
}

// ── 방향 벡터 ──────────────────────────────────────────────────────────────

FVector UCameraComponent::GetForward() const
{
	const float RadYaw = FMath::DegreesToRadians(Yaw);
	const float RadPitch = FMath::DegreesToRadians(Pitch);

	FVector Forward;
	Forward.X = cosf(RadPitch) * cosf(RadYaw);
	Forward.Y = cosf(RadPitch) * sinf(RadYaw);
	Forward.Z = sinf(RadPitch);
	return Forward.GetSafeNormal();
}

FVector UCameraComponent::GetRight() const
{
	return FVector::CrossProduct(Up, GetForward()).GetSafeNormal();
}

// ── 이동 ───────────────────────────────────────────────────────────────────

void UCameraComponent::MoveForward(float Value)
{
	Position += GetForward() * (Value * Speed);
}

void UCameraComponent::MoveRight(float Value)
{
	Position += GetRight() * (Value * Speed);
}

void UCameraComponent::MoveUp(float Value)
{
	Position += Up * (Value * Speed);
}

void UCameraComponent::PanRight(float Value)
{
	Position += GetRight() * (Value * Speed);
}

void UCameraComponent::PanUp(float Value)
{
	const FVector Forward = GetForward();
	const FVector Right = GetRight();
	const FVector LocalUp = FVector::CrossProduct(Forward, Right).GetSafeNormal();
	Position += LocalUp * (Value * Speed);
}

// ── 회전 ───────────────────────────────────────────────────────────────────

void UCameraComponent::Rotate(float DeltaYaw, float DeltaPitch)
{
	Yaw += DeltaYaw;
	Pitch += DeltaPitch;
	Pitch = std::clamp(Pitch, -89.0f, 89.0f);
}

void UCameraComponent::Zoom(float Value)
{
	ZoomDist += Value;
}

void UCameraComponent::SetRotation(float InYaw, float InPitch)
{
	Yaw = InYaw;
	Pitch = std::clamp(InPitch, -89.0f, 89.0f);
}

// ── 행렬 ───────────────────────────────────────────────────────────────────

FMatrix UCameraComponent::GetViewMatrix() const
{
	const FVector Target = Position + GetForward();
	return FMatrix::MakeViewLookAtLH(Position, Target, Up);
}

FMatrix UCameraComponent::GetProjectionMatrix() const
{
	if (bIsOrthographic)
	{
		const float SafeWidth = FMath::Max(OrthoWidth, 0.01f);
		const float SafeAspect = FMath::Max(AspectRatio, 0.01f);
		return FMatrix::MakeOrthographicLH(SafeWidth, SafeWidth / SafeAspect, NearPlane, FarPlane);
	}

	return FMatrix::MakePerspectiveFovLH(
		FMath::DegreesToRadians(FOV), AspectRatio, NearPlane, FarPlane);
}

// ── ViewInfo 스냅샷 ────────────────────────────────────────────────────────

FCameraViewInfo UCameraComponent::GetViewInfo() const
{
	FCameraViewInfo Info;
	Info.Position = Position;
	Info.Forward = GetForward();
	Info.Right = GetRight();
	Info.Up = Up;
	Info.ViewMatrix = GetViewMatrix();
	Info.ProjectionMatrix = GetProjectionMatrix();
	Info.FOV = FOV;
	Info.AspectRatio = AspectRatio;
	Info.OrthoWidth = OrthoWidth;
	Info.OrthoHeight = GetOrthoHeight();
	Info.bIsOrthographic = bIsOrthographic;
	return Info;
}

// ── Setter ─────────────────────────────────────────────────────────────────

void UCameraComponent::SetFOV(float InFOV)
{
	FOV = std::clamp(InFOV, 1.0f, 179.0f);
}

void UCameraComponent::SetAspectRatio(float InAspectRatio)
{
	AspectRatio = FMath::Max(InAspectRatio, 0.01f);
}

void UCameraComponent::SetOrthoWidth(float InOrthoWidth)
{
	OrthoWidth = FMath::Max(InOrthoWidth, 0.01f);
}


//void UCameraComponent::SetFoucs(USceneComponent* InFocusTarget)
//{
//	Camera->SetFocus(InFocusTarget);
//}