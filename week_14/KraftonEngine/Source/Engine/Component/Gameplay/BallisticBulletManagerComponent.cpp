#include "Component/Gameplay/BallisticBulletManagerComponent.h"

#include "Component/Gameplay/SniperWeaponComponent.h"
#include "Core/Types/CollisionTypes.h"
#include "Debug/DrawDebugHelpers.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "Math/Quat.h"

#include <cmath>

#include <algorithm>

namespace
{
	constexpr uint32 SniperBulletQueryObjectMask =
		ObjectTypeBit(ECollisionChannel::WorldStatic) |
		ObjectTypeBit(ECollisionChannel::WorldDynamic) |
		ObjectTypeBit(ECollisionChannel::Pawn);
	constexpr float SniperDebugTrailDuration = 1.5f;
	constexpr float SniperDebugMarkerMinRadius = 0.15f;
	constexpr int32 SniperDebugMarkerSegments = 12;
	constexpr float SniperDebugGravityMultiplier = 1.0f;
	constexpr float SniperBulletMinSweepRadius = 0.01f;
	constexpr float SniperDebugHitMarkerRadius = 0.2f;
	constexpr float SniperWindDebugArrowScale = 3.0f;
	constexpr float SniperWindDebugArrowHeadSize = 0.35f;
	constexpr float SniperWindDebugArrowDuration = 0.0f;
	constexpr float SniperWindDebugMinMagnitude = 0.01f;
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
	const FVector AppliedWindAcceleration = bEnableWind ? WindAcceleration : FVector::ZeroVector;

	if (World)
	{
		DrawWindDebug(World);
	}

	for (FBallisticBullet& Bullet : ActiveBullets)
	{
		UpdateSingleBullet(Bullet, WorldGravity, AppliedWindAcceleration, DeltaTime, World);
	}
}

void UBallisticBulletManagerComponent::UpdateSingleBullet(
	FBallisticBullet& Bullet,
	const FVector& WorldGravity,
	const FVector& AppliedWindAcceleration,
	float DeltaTime,
	UWorld* World)
{
	if (!Bullet.bIsAlive)
	{
		return;
	}

	Bullet.PreviousPosition = Bullet.Position;

	const FVector GravityAcceleration = WorldGravity * Bullet.GravityScale * SniperDebugGravityMultiplier;
	const FVector WindDriftAcceleration = AppliedWindAcceleration * Bullet.WindInfluenceScale;
	const FVector TotalAcceleration = GravityAcceleration + WindDriftAcceleration;
	Bullet.Position += Bullet.Velocity * DeltaTime + TotalAcceleration * (0.5f * DeltaTime * DeltaTime);
	Bullet.Velocity += TotalAcceleration * DeltaTime;
	Bullet.LifeTime -= DeltaTime;

	FHitResult Hit;
	if (World && QueryBulletHit(Bullet, World, Hit))
	{
		HandleBulletHit(Bullet, Hit, World);
	}

	if (World)
	{
		DrawDebugLine(World, Bullet.PreviousPosition, Bullet.Position, FColor(0, 220, 255), SniperDebugTrailDuration);
		DrawDebugSphere(
			World,
			Bullet.Position,
			(std::max)(Bullet.Radius * 3.0f, SniperDebugMarkerMinRadius),
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

void UBallisticBulletManagerComponent::DrawWindDebug(UWorld* World) const
{
	if (!World || !bEnableWind)
	{
		return;
	}

	if (WindAcceleration.Length() <= SniperWindDebugMinMagnitude)
	{
		return;
	}

	const AActor* OwnerActor = GetOwner();
	const FVector ArrowStart = OwnerActor ? OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, 1.5f) : FVector::ZeroVector;
	const FVector ArrowEnd = ArrowStart + (WindAcceleration * SniperWindDebugArrowScale);
	const FVector Direction = (ArrowEnd - ArrowStart).Normalized();
	const FVector UpVector = FVector(0.0f, 0.0f, 1.0f);
	FVector ArrowSide = FVector::Cross(Direction, UpVector);
	if (ArrowSide.IsNearlyZero())
	{
		ArrowSide = FVector(0.0f, 1.0f, 0.0f);
	}
	else
	{
		ArrowSide = ArrowSide.Normalized();
	}

	const FVector ArrowHeadBase = ArrowEnd - Direction * SniperWindDebugArrowHeadSize;
	const FVector ArrowHeadLeft = ArrowHeadBase + ArrowSide * (SniperWindDebugArrowHeadSize * 0.5f);
	const FVector ArrowHeadRight = ArrowHeadBase - ArrowSide * (SniperWindDebugArrowHeadSize * 0.5f);

	DrawDebugLine(World, ArrowStart, ArrowEnd, FColor(80, 255, 120), SniperWindDebugArrowDuration);
	DrawDebugLine(World, ArrowEnd, ArrowHeadLeft, FColor(80, 255, 120), SniperWindDebugArrowDuration);
	DrawDebugLine(World, ArrowEnd, ArrowHeadRight, FColor(80, 255, 120), SniperWindDebugArrowDuration);
}

bool UBallisticBulletManagerComponent::QueryBulletHit(const FBallisticBullet& Bullet, UWorld* World, FHitResult& OutHit) const
{
	if (!World)
	{
		return false;
	}

	const FVector Segment = Bullet.Position - Bullet.PreviousPosition;
	const float SegmentLength = Segment.Length();
	if (SegmentLength <= SniperBulletMinSweepRadius)
	{
		return false;
	}

	if (Bullet.Radius > SniperBulletMinSweepRadius)
	{
		const FCollisionShape SweepShape = FCollisionShape::MakeSphere(Bullet.Radius);
		if (World->PhysicsSweepByObjectTypes(
			Bullet.PreviousPosition,
			Bullet.Position,
			FQuat::Identity,
			SweepShape,
			OutHit,
			SniperBulletQueryObjectMask,
			Bullet.Owner))
		{
			return OutHit.bHit;
		}
	}

	return World->PhysicsRaycastByObjectTypes(
		Bullet.PreviousPosition,
		Segment / SegmentLength,
		SegmentLength,
		OutHit,
		SniperBulletQueryObjectMask,
		Bullet.Owner);
}

void UBallisticBulletManagerComponent::HandleBulletHit(FBallisticBullet& Bullet, const FHitResult& Hit, UWorld* World)
{
	Bullet.Position = Hit.WorldHitLocation;
	Bullet.bIsAlive = false;

	if (World)
	{
		DrawDebugSphere(
			World,
			Hit.WorldHitLocation,
			SniperDebugHitMarkerRadius,
			SniperDebugMarkerSegments,
			FColor(255, 255, 0),
			SniperDebugTrailDuration);
	}

	if (USniperWeaponComponent* SniperWeapon = WeaponComponent.Get())
	{
		SniperWeapon->NotifySniperHit(BuildSniperHitInfo(Bullet, Hit));
	}
}

FSniperHitInfo UBallisticBulletManagerComponent::BuildSniperHitInfo(const FBallisticBullet& Bullet, const FHitResult& Hit) const
{
	FSniperHitInfo HitInfo;
	HitInfo.HitActor = Hit.HitActor;
	HitInfo.HitLocation = Hit.WorldHitLocation;
	HitInfo.HitNormal = !Hit.ImpactNormal.IsNearlyZero() ? Hit.ImpactNormal : Hit.WorldNormal;
	HitInfo.ShotDirection = Bullet.Velocity.IsNearlyZero() ? FVector::ZeroVector : Bullet.Velocity.Normalized();
	HitInfo.Damage = Bullet.Damage;
	HitInfo.AmmoType = Bullet.AmmoType;
	HitInfo.bIsScopedShot = Bullet.bWasScopedShot;
	HitInfo.bIsHeadshot = false;
	HitInfo.bIsArmorPiercing = Bullet.bCanDamageArmor;
	HitInfo.Shooter = Bullet.Owner;
	return HitInfo;
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
