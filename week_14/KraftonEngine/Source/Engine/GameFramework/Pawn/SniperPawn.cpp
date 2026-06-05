#include "GameFramework/Pawn/SniperPawn.h"

#include "Component/Camera/CameraComponent.h"
#include "Component/Gameplay/BallisticBulletManagerComponent.h"
#include "Component/Gameplay/SniperWeaponComponent.h"
#include "Component/SceneComponent.h"

ASniperPawn::ASniperPawn()
{
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
}

void ASniperPawn::BeginPlay()
{
	if (!GetRootComponent())
	{
		InitDefaultComponents();
	}
	else
	{
		CacheComponentReferences();
	}

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
