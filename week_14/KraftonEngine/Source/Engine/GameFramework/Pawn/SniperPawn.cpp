#include "GameFramework/Pawn/SniperPawn.h"

#include "Component/Camera/CameraComponent.h"
#include "Component/Gameplay/BallisticBulletManagerComponent.h"
#include "Component/Gameplay/SniperWeaponComponent.h"
#include "Component/Input/InputComponent.h"
#include "Component/SceneComponent.h"
#include "Math/MathUtils.h"

#include <algorithm>
#include <cmath>

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
	InputComponent->AddActionMapping("SniperFire", "LeftMouseButton");
	InputComponent->AddActionMapping("SniperScope", "RightMouseButton");

	InputComponent->BindAxis("SniperTurn", [this](float Value)
	{
		HandleTurnInput(Value);
	});

	InputComponent->BindAxis("SniperLookUp", [this](float Value)
	{
		HandleLookUpInput(Value);
	});

	InputComponent->BindAction("SniperScope", EInputEvent::Pressed, [this]()
	{
		HandleScopePressed();
	});

	InputComponent->BindAction("SniperFire", EInputEvent::Pressed, [this]()
	{
		HandleFirePressed();
	});

	InputComponent->BindAction("SniperScope", EInputEvent::Released, [this]()
	{
		HandleScopeReleased();
	});
}

void ASniperPawn::Tick(float DeltaTime)
{
	APawn::Tick(DeltaTime);

	UpdateScopeState(DeltaTime);
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
	ScopeState.bIsScoped = false;
	ScopeState.TargetFOV = ScopeState.NormalFOV;
	ScopeState.CurrentFOV = ScopeState.NormalFOV;
	ScopeState.CurrentSensitivity = ScopeState.NormalSensitivity;

	if (Camera)
	{
		Camera->SetFOV(ScopeState.CurrentFOV);
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
	ScopeState.TargetFOV = ScopeState.bIsScoped ? ScopeState.ScopedFOV : ScopeState.NormalFOV;
	ScopeState.CurrentFOV = ExponentialInterpTo(
		ScopeState.CurrentFOV,
		ScopeState.TargetFOV,
		DeltaTime,
		ScopeState.ScopeBlendSpeed);

	const float ScopeRange = ScopeState.NormalFOV - ScopeState.ScopedFOV;
	float ScopeAlpha = 0.0f;
	if (std::abs(ScopeRange) > FMath::Epsilon)
	{
		ScopeAlpha = FMath::Clamp((ScopeState.NormalFOV - ScopeState.CurrentFOV) / ScopeRange, 0.0f, 1.0f);
	}

	ScopeState.CurrentSensitivity = FMath::Lerp(
		ScopeState.NormalSensitivity,
		ScopeState.ScopedSensitivity,
		ScopeAlpha);

	if (Camera)
	{
		Camera->SetFOV(ScopeState.CurrentFOV);
	}
}

void ASniperPawn::ApplySniperControlRotation()
{
	ApplyControllerRotationToRoot();
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

void ASniperPawn::HandleScopePressed()
{
	InputState.bScopeHeld = true;
}

void ASniperPawn::HandleScopeReleased()
{
	InputState.bScopeHeld = false;
}

void ASniperPawn::HandleFirePressed()
{
	InputState.bFirePressed = true;
	FireCurrentRound();
}

bool ASniperPawn::FireCurrentRound()
{
	USniperWeaponComponent* SniperWeapon = WeaponComponent.Get();
	UCameraComponent* SniperCamera = Camera.Get();
	if (!SniperWeapon || !SniperCamera)
	{
		return false;
	}

	const FVector ShotDirection = GetControlRotation().GetForwardVector().Normalized();
	if (ShotDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector MuzzlePosition = SniperCamera->GetWorldLocation() + ShotDirection * 10.0f;
	const bool bFired = SniperWeapon->RequestFire(
		MuzzlePosition,
		ShotDirection,
		InputState.bScopeHeld,
		this);

	InputState.bFirePressed = false;
	return bFired;
}
