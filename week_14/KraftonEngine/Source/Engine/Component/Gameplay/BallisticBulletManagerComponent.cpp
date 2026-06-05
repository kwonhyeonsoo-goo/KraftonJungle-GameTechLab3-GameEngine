#include "Component/Gameplay/BallisticBulletManagerComponent.h"

#include "Component/Gameplay/SniperWeaponComponent.h"
#include "Debug/DrawDebugHelpers.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"

#include <cmath>

#include <algorithm>

namespace
{
	constexpr float SniperDebugTrailDuration = 1.5f;
	constexpr float SniperDebugMarkerMinRadius = 8.0f;
	constexpr int32 SniperDebugMarkerSegments = 12;
	constexpr float SniperDebugGravityMultiplier = 1.0f;
}

UBallisticBulletManagerComponent::UBallisticBulletManagerComponent()
{
	bTickEnable = true;
}

void UBallisticBulletManagerComponent::BeginPlay()
{
	UActorComponent::BeginPlay();
	ResolveWeaponComponent();
}

void UBallisticBulletManagerComponent::EndPlay()
{
	ResetBullets();
	UActorComponent::EndPlay();
}

bool UBallisticBulletManagerComponent::SpawnBullet(const FBallisticBullet& Bullet)
{
	if (!Bullet.bIsAlive)
	{
		return false;
	}

	ActiveBullets.push_back(Bullet);
	return true;
}

void UBallisticBulletManagerComponent::ResetBullets()
{
	ActiveBullets.clear();
}

void UBallisticBulletManagerComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction& ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
	(void)TickType;
	(void)ThisTickFunction;

	UpdateBullets(DeltaTime);
	CompactDeadBullets();
}

void UBallisticBulletManagerComponent::UpdateBullets(float DeltaTime)
{
	if (DeltaTime <= 0.0f || ActiveBullets.empty())
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
	const FVector WorldGravity = World ? World->GetWorldSettings().Gravity : FVector(0.0f, 0.0f, -9.81f);

	for (FBallisticBullet& Bullet : ActiveBullets)
	{
		UpdateSingleBullet(Bullet, WorldGravity, DeltaTime, World);
	}
}

void UBallisticBulletManagerComponent::UpdateSingleBullet(
	FBallisticBullet& Bullet,
	const FVector& WorldGravity,
	float DeltaTime,
	UWorld* World)
{
	if (!Bullet.bIsAlive)
	{
		return;
	}

	Bullet.PreviousPosition = Bullet.Position;

	const FVector GravityAcceleration = WorldGravity * Bullet.GravityScale * SniperDebugGravityMultiplier;
	Bullet.Position += Bullet.Velocity * DeltaTime + GravityAcceleration * (0.5f * DeltaTime * DeltaTime);
	Bullet.Velocity += GravityAcceleration * DeltaTime;
	Bullet.LifeTime -= DeltaTime;

	if (World)
	{
		DrawDebugLine(World, Bullet.PreviousPosition, Bullet.Position, FColor(0, 220, 255), SniperDebugTrailDuration);
		DrawDebugSphere(
			World,
			Bullet.Position,
			(std::max)(Bullet.Radius * 2.0f, SniperDebugMarkerMinRadius),
			SniperDebugMarkerSegments,
			FColor(255, 80, 80),
			SniperDebugTrailDuration);
	}

	const bool bExpired = Bullet.LifeTime <= 0.0f;
	const bool bInvalidPosition =
		!std::isfinite(Bullet.Position.X) ||
		!std::isfinite(Bullet.Position.Y) ||
		!std::isfinite(Bullet.Position.Z);

	if (bExpired || bInvalidPosition)
	{
		Bullet.bIsAlive = false;
	}
}

void UBallisticBulletManagerComponent::CompactDeadBullets()
{
	ActiveBullets.erase(
		std::remove_if(
			ActiveBullets.begin(),
			ActiveBullets.end(),
			[](const FBallisticBullet& Bullet)
			{
				return !Bullet.bIsAlive;
			}),
		ActiveBullets.end());
}

void UBallisticBulletManagerComponent::ResolveWeaponComponent()
{
	if (USniperWeaponComponent* Existing = WeaponComponent.Get())
	{
		if (Existing->GetOwner() == GetOwner())
		{
			return;
		}
	}

	WeaponComponent = GetOwner() ? GetOwner()->GetComponentByClass<USniperWeaponComponent>() : nullptr;
}
