#include "Camera.h"

#include <algorithm>
#include <cmath>

#include "Input/Input.h"
#include "Math/MathUtility.h"

namespace
{
	constexpr float DegreesToRadians = 3.14159265358979323846f / 180.0f;
	constexpr float RadiansToDegrees = 180.0f / 3.14159265358979323846f;
	constexpr float MinFovDegrees = 1.0f;
	constexpr float MaxFovDegrees = 179.0f;
	constexpr float MinAspectRatio = 0.01f;
	constexpr float MinClipDistance = 0.001f;
	constexpr float MinClipRange = 0.001f;
	constexpr float MaxPitchDegrees = 89.0f;

	void BuildCameraBasisFromViewAngles(
		float InYawDegrees,
		float InPitchDegrees,
		FVector& OutForward,
		FVector& OutRight,
		FVector& OutUp) noexcept
	{
		const float YawRadians = InYawDegrees * DegreesToRadians;
		const float PitchRadians = InPitchDegrees * DegreesToRadians;

		const float CosYaw = std::cos(YawRadians);
		const float SinYaw = std::sin(YawRadians);
		const float CosPitch = std::cos(PitchRadians);
		const float SinPitch = std::sin(PitchRadians);

		OutForward = FVector(
			CosPitch * CosYaw,
			CosPitch * SinYaw,
			SinPitch).GetSafeNormal();

		// Keep right locked to the yaw-only horizon so pitch never introduces roll.
		OutRight = FVector(-SinYaw, CosYaw, 0.0f).GetSafeNormal();
		OutUp = FVector::CrossProduct(OutForward, OutRight).GetSafeNormal();
	}
}

void FCamera::SetTransform(const FTransform& InTransform) noexcept
{
	Transform = InTransform;
	CurrentLocation = InTransform.GetLocation();
	CacheViewAnglesFromRotation(InTransform.GetRotation());
	SyncTransformFromState();
}

void FCamera::SetLocation(const FVector& InLocation) noexcept
{
	CurrentLocation = InLocation;
	Transform.SetLocation(CurrentLocation);
}

void FCamera::SetRotation(const FQuat& InRotation) noexcept
{
	CacheViewAnglesFromRotation(InRotation);
	SyncTransformFromState();
}

void FCamera::SetRotation(const FRotator& InRotation) noexcept
{
	SetRotation(InRotation.Quaternion());
}

FMatrix FCamera::GetViewMatrix() const
{
	const FVector Eye = CurrentLocation;
	const FQuat RotationQuat = Transform.GetRotation().GetNormalized();
	const FVector Forward = RotationQuat.GetForwardVector();
	const FVector Up = RotationQuat.GetUpVector();
	return FMatrix::MakeViewLookAtLH(Eye, Eye + Forward, Up);
}

FMatrix FCamera::GetProjectionMatrix() const
{
	return FMatrix::MakePerspectiveFovLH(FovDegrees * DegreesToRadians, AspectRatio, NearClip, FarClip);
}

void FCamera::SetFOV(float InFov)
{
	FovDegrees = std::clamp(InFov, MinFovDegrees, MaxFovDegrees);
}

void FCamera::SetAspectRatio(float InAspectRatio)
{
	AspectRatio = std::max(InAspectRatio, MinAspectRatio);
}

void FCamera::SetNearClip(float InNearClip)
{
	NearClip = std::max(InNearClip, MinClipDistance);
	if (FarClip <= NearClip)
	{
		FarClip = NearClip + MinClipRange;
	}
}

void FCamera::SetFarClip(float InFarClip)
{
	FarClip = std::max(InFarClip, NearClip + MinClipRange);
}

void FCamera::SetSpeed(float InSpeed)
{
	MovementSpeed = std::max(InSpeed, 0.0f);
}

void FCamera::SetSensitivity(float InSensitivity)
{
	LookSensitivity = std::max(InSensitivity, 0.0f);
}

void FCamera::Update(const FInput& Input, float DeltaTime)
{
	if (Input.IsMouseButtonDown(FInput::MOUSE_RIGHT))
	{
		const float DeltaYaw = Input.GetMouseDeltaX() * LookSensitivity;
		const float DeltaPitch = -Input.GetMouseDeltaY() * LookSensitivity;
		Rotate(DeltaYaw, DeltaPitch);
	}

	ApplyMovementInput(
		Input.IsKeyDown('W'),
		Input.IsKeyDown('S'),
		Input.IsKeyDown('D'),
		Input.IsKeyDown('A'),
		Input.IsKeyDown(VK_SPACE) || Input.IsKeyDown('E'),
		Input.IsKeyDown(VK_CONTROL) || Input.IsKeyDown('Q'),
		DeltaTime);
}

void FCamera::MoveForward(float Value)
{
	if (Value == 0.0f)
	{
		return;
	}

	const FQuat RotationQuat = Transform.GetRotation().GetNormalized();
	CurrentLocation += RotationQuat.GetForwardVector() * Value;
	Transform.SetLocation(CurrentLocation);
}

void FCamera::MoveRight(float Value)
{
	if (Value == 0.0f)
	{
		return;
	}

	const FQuat RotationQuat = Transform.GetRotation().GetNormalized();
	CurrentLocation += RotationQuat.GetRightVector() * Value;
	Transform.SetLocation(CurrentLocation);
}

void FCamera::MoveUp(float Value)
{
	if (Value == 0.0f)
	{
		return;
	}

	CurrentLocation += FVector::UpVector * Value;
	Transform.SetLocation(CurrentLocation);
}

void FCamera::Rotate(float DeltaYaw, float DeltaPitch)
{
	if (DeltaYaw == 0.0f && DeltaPitch == 0.0f)
	{
		return;
	}

	CurrentYawDegrees = FRotator::NormalizeAxis(CurrentYawDegrees + DeltaYaw);
	CurrentPitchDegrees = std::clamp(CurrentPitchDegrees + DeltaPitch, -MaxPitchDegrees, MaxPitchDegrees);
	SyncTransformFromState();
}

void FCamera::ApplyMovementInput(bool bMoveForward, bool bMoveBackward, bool bMoveRight, bool bMoveLeft, bool bMoveUp, bool bMoveDown, float DeltaTime)
{
	if (DeltaTime <= 0.0f || MovementSpeed <= 0.0f)
	{
		return;
	}

	const float Distance = MovementSpeed * DeltaTime;
	const float ForwardInput = (bMoveForward ? 1.0f : 0.0f) - (bMoveBackward ? 1.0f : 0.0f);
	const float RightInput = (bMoveRight ? 1.0f : 0.0f) - (bMoveLeft ? 1.0f : 0.0f);
	const float UpInput = (bMoveUp ? 1.0f : 0.0f) - (bMoveDown ? 1.0f : 0.0f);

	MoveForward(ForwardInput * Distance);
	MoveRight(RightInput * Distance);
	MoveUp(UpInput * Distance);
}

void FCamera::CacheViewAnglesFromRotation(const FQuat& InRotation) noexcept
{
	const FVector Forward = InRotation.GetNormalized().GetForwardVector().GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		CurrentPitchDegrees = 0.0f;
		CurrentYawDegrees = 0.0f;
		return;
	}

	const float HorizontalLength = std::sqrt(Forward.X * Forward.X + Forward.Y * Forward.Y);
	CurrentPitchDegrees = std::clamp(std::atan2(Forward.Z, HorizontalLength) * RadiansToDegrees, -MaxPitchDegrees, MaxPitchDegrees);
	CurrentYawDegrees = FRotator::NormalizeAxis(std::atan2(Forward.Y, Forward.X) * RadiansToDegrees);
}

void FCamera::SyncTransformFromState() noexcept
{
	FVector Forward;
	FVector Right;
	FVector Up;
	BuildCameraBasisFromViewAngles(CurrentYawDegrees, CurrentPitchDegrees, Forward, Right, Up);

	FMatrix RotationMatrix = FMatrix::Identity;
	RotationMatrix.SetAxes(Forward, Right, Up);

	const FQuat ViewRotation = FQuat(RotationMatrix).GetNormalized();
	Transform.SetLocation(CurrentLocation);
	Transform.SetRotation(ViewRotation);
}
