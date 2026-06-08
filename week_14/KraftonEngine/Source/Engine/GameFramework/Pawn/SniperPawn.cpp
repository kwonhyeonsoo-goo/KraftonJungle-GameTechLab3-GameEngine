#include "GameFramework/Pawn/SniperPawn.h"

#include "Component/Camera/CameraComponent.h"
#include "Component/Gameplay/BallisticBulletManagerComponent.h"
#include "Component/Gameplay/SniperWeaponComponent.h"
#include "Component/Input/InputComponent.h"
#include "Component/SceneComponent.h"
#include "GameFramework/Actor/SniperKillCamDirector.h"
#include "GameFramework/Camera/PlayerCameraManager.h"
#include "GameFramework/GameMode/PlayerController.h"
#include "Input/InputSystem.h"
#include "Math/MathUtils.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
	float ClampSniperPitch(float Value, float MinPitch, float MaxPitch)
	{
		if (MinPitch > MaxPitch)
		{
			std::swap(MinPitch, MaxPitch);
		}

		return std::clamp(Value, MinPitch, MaxPitch);
	}

	float ExponentialInterpTo(float Current, float Target, float DeltaTime, float Speed)
	{
		if (DeltaTime <= 0.0f || Speed <= 0.0f)
		{
			return Current;
		}

		const float Alpha = 1.0f - std::exp(-Speed * DeltaTime);
		return Current + (Target - Current) * Alpha;
	}

	float ComputeScopeAlpha(const FScopeState& ScopeState)
	{
		const float ScopeRange = ScopeState.NormalFOV - ScopeState.ScopedFOV;
		if (std::abs(ScopeRange) <= FMath::Epsilon)
		{
			return 0.0f;
		}

		return FMath::Clamp((ScopeState.NormalFOV - ScopeState.CurrentFOV) / ScopeRange, 0.0f, 1.0f);
	}

	float RandomRange(float MinValue, float MaxValue)
	{
		const float Alpha = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
		return MinValue + (MaxValue - MinValue) * Alpha;
	}

	bool IsSniperKillCamPlaying(const AActor* Actor)
	{
		return Actor && ASniperKillCamDirector::IsPlayingInWorld(Actor->GetWorld());
	}
}

ASniperPawn::ASniperPawn()
{
	bNeedsTick = true;
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
}

void ASniperPawn::BeginPlay()
{
	CacheComponentReferences();
	InitDefaultComponents();
	CacheComponentReferences();
	SyncSniperRuntimeState();

	APawn::BeginPlay();
}

void ASniperPawn::EndPlay()
{
	if (APlayerController* PC = GetController())
	{
		if (APlayerCameraManager* CameraManager = PC->GetPlayerCameraManager())
		{
			CameraManager->SetScopeZoomEnabled(false);
			CameraManager->ClearScopeLens();
		}
	}

	AActor::EndPlay();
}

void ASniperPawn::PostDuplicate()
{
	APawn::PostDuplicate();
	CacheComponentReferences();
}

void ASniperPawn::SetupInputComponent()
{
	APawn::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->AddMouseAxisMapping("SniperTurn", EInputAxisSourceType::MouseX, 1.0f);
	InputComponent->AddMouseAxisMapping("SniperLookUp", EInputAxisSourceType::MouseY, 1.0f);
	InputComponent->AddMouseAxisMapping("SniperScopeZoom", EInputAxisSourceType::MouseWheel, 1.0f);
	InputComponent->AddGamepadAxisMapping("SniperGamepadTurn", EInputAxisSourceType::GamepadLeftStickX, 1.0f);
	InputComponent->AddGamepadAxisMapping("SniperGamepadLookUp", EInputAxisSourceType::GamepadLeftStickY, 1.0f);
	InputComponent->AddGamepadAxisMapping("SniperGamepadScope", EInputAxisSourceType::GamepadLeftTrigger, 1.0f);
	InputComponent->AddGamepadAxisMapping("SniperGamepadFire", EInputAxisSourceType::GamepadRightTrigger, 1.0f);
	InputComponent->AddActionMapping("SniperFire", "LeftMouseButton");
	InputComponent->AddActionMapping("SniperScope", "RightMouseButton");
	InputComponent->AddActionMapping("SniperHoldBreath", "Shift");
	InputComponent->AddActionMapping("SniperHoldBreath", "LeftShift");
	InputComponent->AddActionMapping("SniperHoldBreath", "RightShift");
	InputComponent->AddActionMapping("SniperSwitchAmmoNormal", "1");
	InputComponent->AddActionMapping("SniperSwitchAmmoAntiMaterial", "2");
	InputComponent->AddActionMapping("SniperReload", "R");
	InputComponent->AddGamepadActionMapping("SniperGamepadHoldBreath", EGamepadButton::LeftShoulder);
	InputComponent->AddGamepadActionMapping("SniperGamepadSwitchAmmoNormal", EGamepadButton::DPadLeft);
	InputComponent->AddGamepadActionMapping("SniperGamepadSwitchAmmoAntiMaterial", EGamepadButton::DPadRight);
	InputComponent->AddGamepadActionMapping("SniperGamepadZoomIn", EGamepadButton::DPadUp);
	InputComponent->AddGamepadActionMapping("SniperGamepadZoomOut", EGamepadButton::DPadDown);
	InputComponent->AddGamepadActionMapping("SniperReload", EGamepadButton::FaceLeft);

	InputComponent->BindAxis("SniperTurn", [this](float Value)
	{
		HandleTurnInput(Value);
	});

	InputComponent->BindAxis("SniperLookUp", [this](float Value)
	{
		HandleLookUpInput(Value);
	});

	InputComponent->BindAxis("SniperGamepadTurn", [this](float Value)
	{
		HandleGamepadTurnInput(Value);
	});

	InputComponent->BindAxis("SniperGamepadLookUp", [this](float Value)
	{
		HandleGamepadLookUpInput(Value);
	});

	InputComponent->BindAxis("SniperScopeZoom", [this](float Value)
	{
		HandleScopeZoomAxis(Value);
	});

	InputComponent->BindAxis("SniperGamepadScope", [this](float Value)
	{
		HandleGamepadScopeAxis(Value);
	});

	InputComponent->BindAxis("SniperGamepadFire", [this](float Value)
	{
		HandleGamepadFireAxis(Value);
	});

	InputComponent->BindAction("SniperScope", EInputEvent::Pressed, [this]()
	{
		HandleScopePressed();
	});

	InputComponent->BindAction("SniperFire", EInputEvent::Pressed, [this]()
	{
		HandleFirePressed();
	});

	InputComponent->BindAction("SniperHoldBreath", EInputEvent::Pressed, [this]()
	{
		HandleHoldBreathPressed();
	});

	InputComponent->BindAction("SniperHoldBreath", EInputEvent::Released, [this]()
	{
		HandleHoldBreathReleased();
	});

	InputComponent->BindAction("SniperGamepadHoldBreath", EInputEvent::Pressed, [this]()
	{
		HandleGamepadHoldBreathPressed();
	});

	InputComponent->BindAction("SniperGamepadHoldBreath", EInputEvent::Released, [this]()
	{
		HandleGamepadHoldBreathReleased();
	});

	InputComponent->BindAction("SniperSwitchAmmoNormal", EInputEvent::Pressed, [this]()
	{
		HandleSwitchAmmoNormalPressed();
	});

	InputComponent->BindAction("SniperSwitchAmmoAntiMaterial", EInputEvent::Pressed, [this]()
	{
		HandleSwitchAmmoAntiMaterialPressed();
	});

	InputComponent->BindAction("SniperGamepadSwitchAmmoNormal", EInputEvent::Pressed, [this]()
	{
		HandleSwitchAmmoNormalPressed();
	});

	InputComponent->BindAction("SniperGamepadSwitchAmmoAntiMaterial", EInputEvent::Pressed, [this]()
	{
		HandleSwitchAmmoAntiMaterialPressed();
	});

	InputComponent->BindAction("SniperGamepadZoomIn", EInputEvent::Pressed, [this]()
	{
		HandleScopeZoomInPressed();
	});

	InputComponent->BindAction("SniperGamepadZoomOut", EInputEvent::Pressed, [this]()
	{
		HandleScopeZoomOutPressed();
	});

	InputComponent->BindAction("SniperReload", EInputEvent::Pressed, [this]()
	{
		HandleReloadPressed();
	});

	InputComponent->BindAction("SniperScope", EInputEvent::Released, [this]()
	{
		HandleScopeReleased();
	});
}

void ASniperPawn::ProcessPlayerInput(const FInputSystemSnapshot& Snapshot, float DeltaTime)
{
	CachedInputDeltaTime = DeltaTime > 0.0f ? DeltaTime : (1.0f / 60.0f);
	APawn::ProcessPlayerInput(Snapshot, DeltaTime);
}

void ASniperPawn::Tick(float DeltaTime)
{
	APawn::Tick(DeltaTime);

	if (IsSniperKillCamPlaying(this))
	{
		ForceScopeReleased();
	}

	UpdateScopeState(DeltaTime);
	UpdateHoldBreathState(DeltaTime);
	UpdateAimSwayState(DeltaTime);
	UpdateRecoilState(DeltaTime);
	ApplySniperControlRotation();

	InputState.MouseDeltaX = 0.0f;
	InputState.MouseDeltaY = 0.0f;
}

void ASniperPawn::InitDefaultComponents()
{
	if (!GetRootComponent())
	{
		SniperRoot = AddComponent<USceneComponent>();
		SetRootComponent(SniperRoot.Get());
	}
	else
	{
		SniperRoot = GetRootComponent();
	}

	if (!Camera)
	{
		Camera = AddComponent<UCameraComponent>();
		if (Camera)
		{
			Camera->AttachToComponent(GetRootComponent());
		}
	}

	if (!WeaponComponent)
	{
		WeaponComponent = AddComponent<USniperWeaponComponent>();
	}

	if (!BulletManagerComponent)
	{
		BulletManagerComponent = AddComponent<UBallisticBulletManagerComponent>();
	}
}

void ASniperPawn::CacheComponentReferences()
{
	SniperRoot = GetRootComponent();
	Camera = GetComponentByClass<UCameraComponent>();
	WeaponComponent = GetComponentByClass<USniperWeaponComponent>();
	BulletManagerComponent = GetComponentByClass<UBallisticBulletManagerComponent>();
}

void ASniperPawn::SyncSniperRuntimeState()
{
	InputState = FSniperInputState{};
	CachedInputDeltaTime = 1.0f / 60.0f;
	bMouseScopeInputHeld = false;
	bGamepadScopeInputHeld = false;
	bKeyboardHoldBreathInputHeld = false;
	bGamepadHoldBreathInputHeld = false;
	bGamepadFireTriggerHeld = false;
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;

	if (Camera)
	{
		ScopeState.NormalFOV = Camera->GetFOV();
	}

	ScopeState.MinZoomMagnification = (std::max)(ScopeState.MinZoomMagnification, 1.0f);
	ScopeState.MaxZoomMagnification = (std::max)(ScopeState.MaxZoomMagnification, ScopeState.MinZoomMagnification);
	ScopeState.ZoomStep = (std::max)(ScopeState.ZoomStep, 0.1f);
	ScopeState.ScopedSensitivity = (std::max)(ScopeState.ScopedSensitivity, 0.01f);
	ScopeState.MaxZoomScopedSensitivity = (std::max)(ScopeState.MaxZoomScopedSensitivity, 0.01f);
	GamepadLookSensitivity = (std::max)(GamepadLookSensitivity, 0.0f);
	GamepadTriggerPressThreshold = FMath::Clamp(GamepadTriggerPressThreshold, 0.01f, 1.0f);
	ScopeState.DefaultZoomMagnification = ClampScopeZoomMagnification(ScopeState.DefaultZoomMagnification);
	ScopeState.CurrentZoomMagnification = ScopeState.DefaultZoomMagnification;
	ScopeState.TargetZoomMagnification = ScopeState.DefaultZoomMagnification;
	ScopeState.ScopedFOV = ComputeScopedFOVForMagnification(ScopeState.CurrentZoomMagnification);

	ScopeState.bIsScoped = false;
	ScopeState.TargetFOV = ScopeState.NormalFOV;
	ScopeState.CurrentFOV = ScopeState.NormalFOV;
	ScopeState.CurrentSensitivity = ScopeState.NormalSensitivity;
	AimSwayState.Time = 0.0f;
	AimSwayState.CurrentSwayPitch = 0.0f;
	AimSwayState.CurrentSwayYaw = 0.0f;
	AimSwayState.BreathMultiplier = 1.0f;
	AimSwayState.HoldBreathGauge = AimSwayState.MaxHoldBreathGauge;
	AimSwayState.bForcedRecovery = false;
	AimSwayState.bRequireHoldBreathRelease = false;
	RecoilState = FRecoilState{};

	if (Camera)
	{
		Camera->SetFOV(ScopeState.NormalFOV);
	}

	if (APlayerController* PC = GetController())
	{
		if (APlayerCameraManager* CameraManager = PC->GetPlayerCameraManager())
		{
			CameraManager->SetScopeLensProfile(
				ScopeLensRadius,
				ScopeLensOuterBlurRadius,
				ScopeState.CurrentFOV,
				ScopeLensFeather,
				ScopeLensEdgeBlurRadius,
				ScopeLensIntensity,
				ScopeState.CurrentSensitivity,
				ScopeLensBlendTime,
				0.5f,
				0.5f,
				ScopeLensCenterOffsetX,
				ScopeLensCenterOffsetY);
			CameraManager->SetScopeZoomEnabled(false);
		}
	}

	FRotator Control = GetControlRotation();
	Control.Pitch = ClampSniperPitch(Control.Pitch, MinCameraPitch, MaxCameraPitch);
	Control.Roll = 0.0f;
	SetControlRotation(Control);
	ApplySniperControlRotation();
}

void ASniperPawn::UpdateScopeState(float DeltaTime)
{
	ScopeState.bIsScoped = InputState.bScopeHeld;
	ScopeState.TargetZoomMagnification = ClampScopeZoomMagnification(ScopeState.TargetZoomMagnification);
	ScopeState.CurrentZoomMagnification = ScopeState.TargetZoomMagnification;
	ScopeState.ScopedFOV = ComputeScopedFOVForMagnification(ScopeState.CurrentZoomMagnification);
	const float ScopedSensitivityForCurrentZoom =
		ComputeScopedSensitivityForMagnification(ScopeState.CurrentZoomMagnification);
	ScopeState.TargetFOV = ScopeState.bIsScoped ? ScopeState.ScopedFOV : ScopeState.NormalFOV;
	ScopeState.CurrentFOV = ExponentialInterpTo(
		ScopeState.CurrentFOV,
		ScopeState.TargetFOV,
		DeltaTime,
		ScopeState.ScopeBlendSpeed);

	const float ScopeAlpha = ComputeScopeAlpha(ScopeState);

	ScopeState.CurrentSensitivity = FMath::Lerp(
		ScopeState.NormalSensitivity,
		ScopedSensitivityForCurrentZoom,
		ScopeAlpha);

	if (Camera)
	{
		Camera->SetFOV(ScopeState.NormalFOV);
	}

	if (APlayerController* PC = GetController())
	{
		if (APlayerCameraManager* CameraManager = PC->GetPlayerCameraManager())
		{
			CameraManager->SetScopeLensProfile(
				ScopeLensRadius,
				ScopeLensOuterBlurRadius,
				ScopeState.CurrentFOV,
				ScopeLensFeather,
				ScopeLensEdgeBlurRadius,
				ScopeLensIntensity,
				ScopeState.CurrentSensitivity,
				ScopeLensBlendTime,
				0.5f,
				0.5f,
				ScopeLensCenterOffsetX,
				ScopeLensCenterOffsetY);
			CameraManager->SetScopeZoomEnabled(ScopeState.bIsScoped);
		}
	}

}

void ASniperPawn::UpdateHoldBreathState(float DeltaTime)
{
	if (!InputState.bHoldBreathHeld)
	{
		AimSwayState.bRequireHoldBreathRelease = false;
	}

	const bool bCanHoldBreath =
		InputState.bHoldBreathHeld &&
		ScopeState.bIsScoped &&
		!AimSwayState.bForcedRecovery &&
		!AimSwayState.bRequireHoldBreathRelease &&
		AimSwayState.HoldBreathGauge > 0.0f;

	if (bCanHoldBreath)
	{
		AimSwayState.HoldBreathGauge = FMath::Clamp(
			AimSwayState.HoldBreathGauge - AimSwayState.HoldBreathConsumeSpeed * DeltaTime,
			0.0f,
			AimSwayState.MaxHoldBreathGauge);

		if (AimSwayState.HoldBreathGauge <= 0.0f)
		{
			AimSwayState.bForcedRecovery = true;
			AimSwayState.bRequireHoldBreathRelease = true;
		}
	}
	else
	{
		AimSwayState.HoldBreathGauge = FMath::Clamp(
			AimSwayState.HoldBreathGauge + AimSwayState.HoldBreathRecoverSpeed * DeltaTime,
			0.0f,
			AimSwayState.MaxHoldBreathGauge);
	}

	float TargetBreathMultiplier = 1.0f;
	if (AimSwayState.bForcedRecovery)
	{
		const float RecoveryRatio = AimSwayState.MaxHoldBreathGauge > 0.0f
			? FMath::Clamp(AimSwayState.HoldBreathGauge / AimSwayState.MaxHoldBreathGauge, 0.0f, 1.0f)
			: 1.0f;
		TargetBreathMultiplier = FMath::Lerp(ExhaustedSwayMultiplier, 1.0f, RecoveryRatio);
	}
	else if (bCanHoldBreath)
	{
		TargetBreathMultiplier = HoldBreathSwayMultiplier;
	}

	if (HoldBreathSwayBlendSpeed <= 0.0f)
	{
		AimSwayState.BreathMultiplier = TargetBreathMultiplier;
	}
	else
	{
		AimSwayState.BreathMultiplier = ExponentialInterpTo(
			AimSwayState.BreathMultiplier,
			TargetBreathMultiplier,
			DeltaTime,
			HoldBreathSwayBlendSpeed);
	}

	if (AimSwayState.bForcedRecovery &&
		AimSwayState.HoldBreathGauge >= AimSwayState.MaxHoldBreathGauge - 0.001f &&
		std::abs(AimSwayState.BreathMultiplier - 1.0f) <= 0.05f)
	{
		AimSwayState.bForcedRecovery = false;
	}
}

void ASniperPawn::UpdateAimSwayState(float DeltaTime)
{
	AimSwayState.Time += DeltaTime;

	const float ScopeAlpha = ComputeScopeAlpha(ScopeState);
	const float BaseAmplitude = FMath::Lerp(
		AimSwayState.BaseSwayAmount * FMath::RadToDeg,
		AimSwayState.ScopedSwayAmount * FMath::RadToDeg,
		ScopeAlpha);
	const float SwayAmplitude = BaseAmplitude * AimSwayState.BreathMultiplier;

	AimSwayState.CurrentSwayPitch = std::sin(AimSwayState.Time * SwayPitchFrequency) * SwayAmplitude;
	AimSwayState.CurrentSwayYaw = std::cos(AimSwayState.Time * SwayYawFrequency) * SwayAmplitude * 0.85f;
}

void ASniperPawn::UpdateRecoilState(float DeltaTime)
{
	RecoilState.CurrentRecoilPitch = ExponentialInterpTo(
		RecoilState.CurrentRecoilPitch,
		0.0f,
		DeltaTime,
		RecoilState.RecoilRecoverSpeed);
	RecoilState.CurrentRecoilYaw = ExponentialInterpTo(
		RecoilState.CurrentRecoilYaw,
		0.0f,
		DeltaTime,
		RecoilState.RecoilRecoverSpeed);
}

void ASniperPawn::ApplySniperControlRotation()
{
	USceneComponent* Root = GetRootComponent();
	if (!Root)
	{
		return;
	}

	FRotator AppliedRotation = Root->GetRelativeRotation();
	const FRotator EffectiveRotation = BuildEffectiveAimRotation();

	if (bUseControllerRotationYaw)
	{
		AppliedRotation.Yaw = EffectiveRotation.Yaw;
	}
	if (bUseControllerRotationPitch)
	{
		AppliedRotation.Pitch = EffectiveRotation.Pitch;
	}
	if (bUseControllerRotationRoll)
	{
		AppliedRotation.Roll = EffectiveRotation.Roll;
	}

	Root->SetRelativeRotation(AppliedRotation);
}

float ASniperPawn::GetScopeBlendAlpha() const
{
	return ComputeScopeAlpha(ScopeState);
}

void ASniperPawn::ForceScopeReleased()
{
	bMouseScopeInputHeld = false;
	bGamepadScopeInputHeld = false;
	bKeyboardHoldBreathInputHeld = false;
	bGamepadHoldBreathInputHeld = false;
	bGamepadFireTriggerHeld = false;
	InputState.bScopeHeld = false;
	InputState.bHoldBreathHeld = false;
	ScopeState.bIsScoped = false;
	ScopeState.TargetFOV = ScopeState.NormalFOV;
	ScopeState.CurrentFOV = ScopeState.NormalFOV;
	ScopeState.CurrentSensitivity = ScopeState.NormalSensitivity;
	AimSwayState.BreathMultiplier = 1.0f;
	AimSwayState.bRequireHoldBreathRelease = false;

	if (Camera)
	{
		Camera->SetFOV(ScopeState.NormalFOV);
	}

	if (APlayerController* PC = GetController())
	{
		if (APlayerCameraManager* CameraManager = PC->GetPlayerCameraManager())
		{
			CameraManager->SetScopeZoomEnabled(false);
			CameraManager->ClearScopeLens();
		}
	}
}

bool ASniperPawn::IsHoldBreathActive() const
{
	return ScopeState.bIsScoped
		&& !AimSwayState.bForcedRecovery
		&& !AimSwayState.bRequireHoldBreathRelease
		&& InputState.bHoldBreathHeld
		&& AimSwayState.HoldBreathGauge > 0.0f
		&& AimSwayState.BreathMultiplier < 1.0f;
}

FRotator ASniperPawn::BuildEffectiveAimRotation() const
{
	FRotator EffectiveRotation = GetControlRotation();
	EffectiveRotation.Pitch += AimSwayState.CurrentSwayPitch + RecoilState.CurrentRecoilPitch;
	EffectiveRotation.Yaw += AimSwayState.CurrentSwayYaw + RecoilState.CurrentRecoilYaw;
	EffectiveRotation.Pitch = ClampSniperPitch(EffectiveRotation.Pitch, MinCameraPitch, MaxCameraPitch);
	EffectiveRotation.Roll = 0.0f;
	return EffectiveRotation;
}

float ASniperPawn::ClampScopeZoomMagnification(float Magnification) const
{
	float MinZoomMagnification = ScopeState.MinZoomMagnification;
	float MaxZoomMagnification = ScopeState.MaxZoomMagnification;
	if (MinZoomMagnification > MaxZoomMagnification)
	{
		std::swap(MinZoomMagnification, MaxZoomMagnification);
	}

	MinZoomMagnification = (std::max)(MinZoomMagnification, 1.0f);
	MaxZoomMagnification = (std::max)(MaxZoomMagnification, MinZoomMagnification);
	return FMath::Clamp(Magnification, MinZoomMagnification, MaxZoomMagnification);
}

float ASniperPawn::ComputeScopedFOVForMagnification(float Magnification) const
{
	const float SafeMagnification = (std::max)(ClampScopeZoomMagnification(Magnification), 1.0f);
	const float HalfBaseScopedFOV = ScopeState.NormalFOV * 0.5f;
	const float ZoomedHalfFOVTangent = std::tan(HalfBaseScopedFOV) / SafeMagnification;
	const float ComputedScopedFOV = std::atan(ZoomedHalfFOVTangent) * 2.0f;
	return FMath::Clamp(ComputedScopedFOV, 0.01f, ScopeState.NormalFOV);
}

float ASniperPawn::ComputeScopedSensitivityForMagnification(float Magnification) const
{
	const float SafeMinZoomMagnification = (std::max)(ScopeState.MinZoomMagnification, 1.0f);
	const float SafeMaxZoomMagnification = (std::max)(ScopeState.MaxZoomMagnification, SafeMinZoomMagnification);
	const float SafeMagnification = ClampScopeZoomMagnification(Magnification);
	const float ZoomRange = SafeMaxZoomMagnification - SafeMinZoomMagnification;
	const float ZoomAlpha = ZoomRange > FMath::Epsilon
		? FMath::Clamp((SafeMagnification - SafeMinZoomMagnification) / ZoomRange, 0.0f, 1.0f)
		: 0.0f;

	const float MinZoomScopedSensitivity = (std::max)(ScopeState.ScopedSensitivity, 0.01f);
	const float MaxZoomScopedSensitivity = (std::max)(ScopeState.MaxZoomScopedSensitivity, 0.01f);
	const float ComputedScopedSensitivity = FMath::Lerp(
		MinZoomScopedSensitivity,
		MaxZoomScopedSensitivity,
		ZoomAlpha);

	return FMath::Clamp(
		ComputedScopedSensitivity,
		0.01f,
		(std::max)(ScopeState.NormalSensitivity, 0.01f));
}

void ASniperPawn::AdjustScopeZoomStep(int32 StepDelta)
{
	if (StepDelta == 0)
	{
		return;
	}

	const float SafeZoomStep = (std::max)(ScopeState.ZoomStep, 0.1f);
	const float NewZoomMagnification =
		ScopeState.TargetZoomMagnification + static_cast<float>(StepDelta) * SafeZoomStep;
	ScopeState.TargetZoomMagnification = ClampScopeZoomMagnification(NewZoomMagnification);
}

void ASniperPawn::HandleTurnInput(float Value)
{
	InputState.MouseDeltaX = Value;
	if (std::abs(Value) <= 0.0001f)
	{
		return;
	}

	FRotator Control = GetControlRotation();
	Control.Yaw += Value * MouseSensitivity * ScopeState.CurrentSensitivity;
	SetControlRotation(Control);
	ApplySniperControlRotation();
}

void ASniperPawn::HandleGamepadTurnInput(float Value)
{
	if (std::abs(Value) <= 0.0001f)
	{
		return;
	}

	FRotator Control = GetControlRotation();
	Control.Yaw += Value * GamepadLookSensitivity * CachedInputDeltaTime * ScopeState.CurrentSensitivity;
	SetControlRotation(Control);
	ApplySniperControlRotation();
}

void ASniperPawn::HandleLookUpInput(float Value)
{
	InputState.MouseDeltaY = Value;
	if (std::abs(Value) <= 0.0001f)
	{
		return;
	}

	const float Direction = bInvertMouseY ? -1.0f : 1.0f;
	FRotator Control = GetControlRotation();
	Control.Pitch += Value * MouseSensitivity * ScopeState.CurrentSensitivity * Direction;
	Control.Pitch = ClampSniperPitch(Control.Pitch, MinCameraPitch, MaxCameraPitch);
	SetControlRotation(Control);
	ApplySniperControlRotation();
}

void ASniperPawn::HandleGamepadLookUpInput(float Value)
{
	if (std::abs(Value) <= 0.0001f)
	{
		return;
	}

	const float Direction = bInvertMouseY ? 1.0f : -1.0f;
	FRotator Control = GetControlRotation();
	Control.Pitch += Value * GamepadLookSensitivity * CachedInputDeltaTime * ScopeState.CurrentSensitivity * Direction;
	Control.Pitch = ClampSniperPitch(Control.Pitch, MinCameraPitch, MaxCameraPitch);
	SetControlRotation(Control);
	ApplySniperControlRotation();
}

void ASniperPawn::HandleScopeZoomAxis(float Value)
{
	if (!ScopeState.bIsScoped || std::abs(Value) <= 0.0001f)
	{
		return;
	}

	AdjustScopeZoomStep(Value > 0.0f ? +1 : -1);
}

void ASniperPawn::HandleGamepadScopeAxis(float Value)
{
	const bool bHeld = Value >= GamepadTriggerPressThreshold;
	if (IsSniperKillCamPlaying(this))
	{
		bGamepadScopeInputHeld = false;
		RefreshScopeHeldState();
		return;
	}

	if (bGamepadScopeInputHeld == bHeld)
	{
		return;
	}

	bGamepadScopeInputHeld = bHeld;
	RefreshScopeHeldState();
}

void ASniperPawn::HandleGamepadFireAxis(float Value)
{
	const bool bPressed = Value >= GamepadTriggerPressThreshold;
	if (bGamepadFireTriggerHeld == bPressed)
	{
		return;
	}

	bGamepadFireTriggerHeld = bPressed;
	if (bPressed)
	{
		HandleFirePressed();
	}
}

void ASniperPawn::HandleScopePressed()
{
	if (IsSniperKillCamPlaying(this))
	{
		bMouseScopeInputHeld = false;
		RefreshScopeHeldState();
		return;
	}

	bMouseScopeInputHeld = true;
	RefreshScopeHeldState();
}

void ASniperPawn::HandleScopeReleased()
{
	bMouseScopeInputHeld = false;
	RefreshScopeHeldState();
}

void ASniperPawn::HandleHoldBreathPressed()
{
	if (IsSniperKillCamPlaying(this))
	{
		bKeyboardHoldBreathInputHeld = false;
		RefreshHoldBreathHeldState();
		return;
	}

	bKeyboardHoldBreathInputHeld = true;
	RefreshHoldBreathHeldState();
}

void ASniperPawn::HandleHoldBreathReleased()
{
	bKeyboardHoldBreathInputHeld = false;
	RefreshHoldBreathHeldState();
}

void ASniperPawn::HandleGamepadHoldBreathPressed()
{
	if (IsSniperKillCamPlaying(this))
	{
		bGamepadHoldBreathInputHeld = false;
		RefreshHoldBreathHeldState();
		return;
	}

	bGamepadHoldBreathInputHeld = true;
	RefreshHoldBreathHeldState();
}

void ASniperPawn::HandleGamepadHoldBreathReleased()
{
	bGamepadHoldBreathInputHeld = false;
	RefreshHoldBreathHeldState();
}

void ASniperPawn::HandleSwitchAmmoNormalPressed()
{
	InputState.bSwitchAmmoPressed = true;
	if (USniperWeaponComponent* SniperWeapon = WeaponComponent.Get())
	{
		SniperWeapon->SetCurrentAmmoType(ESniperAmmoType::Normal);
	}
	InputState.bSwitchAmmoPressed = false;
}

void ASniperPawn::HandleSwitchAmmoAntiMaterialPressed()
{
	InputState.bSwitchAmmoPressed = true;
	if (USniperWeaponComponent* SniperWeapon = WeaponComponent.Get())
	{
		SniperWeapon->SetCurrentAmmoType(ESniperAmmoType::AntiMaterial);
	}
	InputState.bSwitchAmmoPressed = false;
}

void ASniperPawn::HandleFirePressed()
{
	InputState.bFirePressed = true;
	FireCurrentRound();
}

void ASniperPawn::HandleReloadPressed()
{
	if (IsSniperKillCamPlaying(this))
	{
		InputState.bReloadPressed = false;
		return;
	}

	InputState.bReloadPressed = true;
	if (USniperWeaponComponent* SniperWeapon = WeaponComponent.Get())
	{
		SniperWeapon->RequestReload();
	}
	InputState.bReloadPressed = false;
}

void ASniperPawn::HandleScopeZoomInPressed()
{
	if (!ScopeState.bIsScoped)
	{
		return;
	}

	AdjustScopeZoomStep(+1);
}

void ASniperPawn::HandleScopeZoomOutPressed()
{
	if (!ScopeState.bIsScoped)
	{
		return;
	}

	AdjustScopeZoomStep(-1);
}

void ASniperPawn::RefreshScopeHeldState()
{
	InputState.bScopeHeld = bMouseScopeInputHeld || bGamepadScopeInputHeld;
}

void ASniperPawn::RefreshHoldBreathHeldState()
{
	InputState.bHoldBreathHeld = bKeyboardHoldBreathInputHeld || bGamepadHoldBreathInputHeld;
}

void ASniperPawn::ApplyFireRecoil()
{
	USniperWeaponComponent* SniperWeapon = WeaponComponent.Get();
	if (!SniperWeapon)
	{
		return;
	}

	const FAmmoBallisticData* AmmoData = SniperWeapon->GetCurrentAmmoData();
	if (!AmmoData)
	{
		return;
	}

	RecoilState.LastShotRecoilPitch = AmmoData->RecoilPitch;
	RecoilState.LastShotRecoilYaw = RandomRange(-AmmoData->RecoilYawRandomRange, AmmoData->RecoilYawRandomRange);
	RecoilState.CurrentRecoilPitch += RecoilState.LastShotRecoilPitch;
	RecoilState.CurrentRecoilYaw += RecoilState.LastShotRecoilYaw;
}

bool ASniperPawn::FireCurrentRound()
{
	USniperWeaponComponent* SniperWeapon = WeaponComponent.Get();
	UCameraComponent* SniperCamera = Camera.Get();
	if (!SniperWeapon || !SniperCamera)
	{
		return false;
	}

	const FVector ShotDirection = BuildEffectiveAimRotation().GetForwardVector().Normalized();
	if (ShotDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector MuzzlePosition = SniperCamera->GetWorldLocation() + ShotDirection * 5.0f;
	const bool bFired = SniperWeapon->RequestFire(
		MuzzlePosition,
		ShotDirection,
		InputState.bScopeHeld,
		this);

	if (bFired)
	{
		ApplyFireRecoil();
	}

	InputState.bFirePressed = false;
	return bFired;
}
