#include "Component/Gameplay/BallisticBulletManagerComponent.h"

#include "Component/Gameplay/SniperDamageReceiverComponent.h"
#include "Component/Gameplay/SniperWeaponComponent.h"
#include "Component/Primitive/BillboardComponent.h"
#include "Core/Types/CollisionTypes.h"
#include "Debug/DrawDebugHelpers.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialManager.h"
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
	constexpr float SniperRagdollImpactSpeedThreshold = 300.0f;
	constexpr const char* SniperDefaultBulletVisualMaterialPath = "Content/Material/Particle/ParticleSprite.uasset";
	constexpr float SniperBulletVisualMinScale = 0.04f;
	constexpr float SniperBulletTracerMinWidth = 0.01f;
	constexpr float SniperBulletTracerDefaultThickness = 1.0f;
	constexpr float SniperSpeedOfSoundMetersPerSecond = 343.0f;
	constexpr float SniperBaseDragScale = 0.00008f;

	float ComputeMachDragMultiplier(float Speed)
	{
		const float Mach = Speed / SniperSpeedOfSoundMetersPerSecond;
		if (Mach > 1.2f)
		{
			return 1.0f;
		}

		if (Mach > 0.9f)
		{
			const float Alpha = (1.2f - Mach) / 0.3f;
			return FMath::Lerp(1.0f, 1.4f, Alpha);
		}

		return 0.9f;
	}

	FVector ComputeBallisticDragAcceleration(const FBallisticBullet& Bullet)
	{
		const float Speed = Bullet.Velocity.Length();
		if (Speed < 1.0f)
		{
			return FVector::ZeroVector;
		}

		const FVector Direction = Bullet.Velocity / Speed;
		const float SafeBallisticCoefficient = (std::max)(Bullet.BallisticCoefficient, 0.01f);
		const float MachFactor = ComputeMachDragMultiplier(Speed);

		return Direction * -1.0f
			* Speed
			* Speed
			* SniperBaseDragScale
			* MachFactor
			* Bullet.DragScale
			/ SafeBallisticCoefficient;
	}
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
	HideAllBulletVisuals();
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
	HideAllBulletVisuals();
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
	SyncBulletVisuals();
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
	const FVector DragAcceleration = ComputeBallisticDragAcceleration(Bullet);
	const FVector TotalAcceleration = GravityAcceleration + WindDriftAcceleration + DragAcceleration;
	Bullet.Position += Bullet.Velocity * DeltaTime + TotalAcceleration * (0.5f * DeltaTime * DeltaTime);
	Bullet.Velocity += TotalAcceleration * DeltaTime;
	Bullet.LifeTime -= DeltaTime;
	const float SegmentDistance = (Bullet.Position - Bullet.PreviousPosition).Length();

	FHitResult Hit;
	if (World && QueryBulletHit(Bullet, World, Hit))
	{
		HandleBulletHit(Bullet, Hit, World);
	}
	else
	{
		Bullet.TraveledDistance += SegmentDistance;
	}

	if (World && bDrawDebugBallistics)
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

void UBallisticBulletManagerComponent::SyncBulletVisuals()
{
	if (!bEnableBulletVisuals)
	{
		HideAllBulletVisuals();
		return;
	}

	for (int32 BulletIndex = 0; BulletIndex < static_cast<int32>(ActiveBullets.size()); ++BulletIndex)
	{
		UBillboardComponent* HeadVisual = GetOrCreateBulletHeadVisual(BulletIndex);
		UBillboardComponent* TracerVisual = GetOrCreateBulletTracerVisual(BulletIndex);
		if (!HeadVisual || !TracerVisual)
		{
			continue;
		}

		const FBallisticBullet& Bullet = ActiveBullets[BulletIndex];
		const float HeadScale = (std::max)(Bullet.VisualScale, SniperBulletVisualMinScale);
		HeadVisual->SetWorldLocation(Bullet.Position);
		HeadVisual->SetRelativeScale(FVector(1.0f, HeadScale, HeadScale));
		HeadVisual->SetVisibility(Bullet.bIsAlive);

		const FVector Segment = Bullet.Position - Bullet.PreviousPosition;
		const float SegmentDistance = Segment.Length();
		const FVector TracerLocation = Bullet.PreviousPosition + Segment * 0.5f;
		const float TracerWidth = (std::max)(Bullet.VisualTracerWidth, SniperBulletTracerMinWidth);
		float TracerLength = SegmentDistance * Bullet.VisualTracerLengthScale;
		TracerLength = (std::max)(TracerLength, Bullet.VisualTracerMinLength);
		TracerLength = (std::min)(TracerLength, Bullet.VisualTracerMaxLength);
		TracerLength = (std::max)(TracerLength, TracerWidth);

		TracerVisual->SetWorldLocation(TracerLocation);
		TracerVisual->SetRelativeScale(FVector(
			SniperBulletTracerDefaultThickness,
			TracerWidth,
			TracerLength));
		TracerVisual->SetVisibility(Bullet.bIsAlive);
	}

	for (int32 VisualIndex = static_cast<int32>(ActiveBullets.size()); VisualIndex < static_cast<int32>(BulletHeadVisualPool.size()); ++VisualIndex)
	{
		if (UBillboardComponent* Visual = BulletHeadVisualPool[VisualIndex].Get())
		{
			Visual->SetVisibility(false);
		}
	}

	for (int32 VisualIndex = static_cast<int32>(ActiveBullets.size()); VisualIndex < static_cast<int32>(BulletTracerVisualPool.size()); ++VisualIndex)
	{
		if (UBillboardComponent* Visual = BulletTracerVisualPool[VisualIndex].Get())
		{
			Visual->SetVisibility(false);
		}
	}
}

void UBallisticBulletManagerComponent::HideAllBulletVisuals()
{
	for (TWeakObjectPtr<UBillboardComponent>& VisualEntry : BulletHeadVisualPool)
	{
		if (UBillboardComponent* Visual = VisualEntry.Get())
		{
			Visual->SetVisibility(false);
		}
	}

	for (TWeakObjectPtr<UBillboardComponent>& VisualEntry : BulletTracerVisualPool)
	{
		if (UBillboardComponent* Visual = VisualEntry.Get())
		{
			Visual->SetVisibility(false);
		}
	}
}

UBillboardComponent* UBallisticBulletManagerComponent::GetOrCreateBulletHeadVisual(int32 VisualIndex)
{
	if (VisualIndex < 0)
	{
		return nullptr;
	}

	if (VisualIndex < static_cast<int32>(BulletHeadVisualPool.size()))
	{
		if (UBillboardComponent* Existing = BulletHeadVisualPool[VisualIndex].Get())
		{
			return Existing;
		}
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	UBillboardComponent* Visual = OwnerActor->AddComponent<UBillboardComponent>();
	if (!Visual)
	{
		return nullptr;
	}

	if (USceneComponent* RootComponent = OwnerActor->GetRootComponent())
	{
		Visual->AttachToComponent(RootComponent);
	}

	Visual->SetAbsoluteScale(true);
	Visual->SetHiddenInComponentTree(true);
	Visual->SetVisibility(false);

	if (UMaterial* VisualMaterial = ResolveBulletHeadVisualMaterial())
	{
		Visual->SetMaterial(VisualMaterial);
	}

	if (VisualIndex >= static_cast<int32>(BulletHeadVisualPool.size()))
	{
		BulletHeadVisualPool.resize(VisualIndex + 1);
	}

	BulletHeadVisualPool[VisualIndex] = Visual;
	return Visual;
}

UBillboardComponent* UBallisticBulletManagerComponent::GetOrCreateBulletTracerVisual(int32 VisualIndex)
{
	if (VisualIndex < 0)
	{
		return nullptr;
	}

	if (VisualIndex < static_cast<int32>(BulletTracerVisualPool.size()))
	{
		if (UBillboardComponent* Existing = BulletTracerVisualPool[VisualIndex].Get())
		{
			return Existing;
		}
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	UBillboardComponent* Visual = OwnerActor->AddComponent<UBillboardComponent>();
	if (!Visual)
	{
		return nullptr;
	}

	if (USceneComponent* RootComponent = OwnerActor->GetRootComponent())
	{
		Visual->AttachToComponent(RootComponent);
	}

	Visual->SetAbsoluteScale(true);
	Visual->SetHiddenInComponentTree(true);
	Visual->SetVisibility(false);

	if (UMaterial* VisualMaterial = ResolveBulletTracerVisualMaterial())
	{
		Visual->SetMaterial(VisualMaterial);
	}

	if (VisualIndex >= static_cast<int32>(BulletTracerVisualPool.size()))
	{
		BulletTracerVisualPool.resize(VisualIndex + 1);
	}

	BulletTracerVisualPool[VisualIndex] = Visual;
	return Visual;
}

UMaterial* UBallisticBulletManagerComponent::ResolveBulletHeadVisualMaterial()
{
	if (UMaterial* Existing = BulletHeadVisualMaterial.Get())
	{
		return Existing;
	}

	const FString MaterialPath =
		(!BulletHeadVisualMaterialPath.empty() && BulletHeadVisualMaterialPath != "None")
		? static_cast<FString>(BulletHeadVisualMaterialPath)
		: FString(SniperDefaultBulletVisualMaterialPath);

	UMaterial* LoadedMaterial = FMaterialManager::Get().GetOrCreateMaterial(MaterialPath);
	BulletHeadVisualMaterial = LoadedMaterial;
	return LoadedMaterial;
}

UMaterial* UBallisticBulletManagerComponent::ResolveBulletTracerVisualMaterial()
{
	if (UMaterial* Existing = BulletTracerVisualMaterial.Get())
	{
		return Existing;
	}

	const FString MaterialPath =
		(!BulletTracerVisualMaterialPath.empty() && BulletTracerVisualMaterialPath != "None")
		? static_cast<FString>(BulletTracerVisualMaterialPath)
		: FString(SniperDefaultBulletVisualMaterialPath);

	UMaterial* LoadedMaterial = FMaterialManager::Get().GetOrCreateMaterial(MaterialPath);
	BulletTracerVisualMaterial = LoadedMaterial;
	return LoadedMaterial;
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

	if (World && bDrawDebugImpactMarker)
	{
		DrawDebugSphere(
			World,
			Hit.WorldHitLocation,
			SniperDebugHitMarkerRadius,
			SniperDebugMarkerSegments,
			FColor(255, 255, 0),
			SniperDebugTrailDuration);
	}

	FSniperHitInfo HitInfo = BuildSniperHitInfo(Bullet, Hit);
	if (AActor* HitActor = HitInfo.HitActor)
	{
		if (USniperDamageReceiverComponent* DamageReceiver = HitActor->GetComponentByClass<USniperDamageReceiverComponent>())
		{
			HitInfo = DamageReceiver->ResolveSniperHit(HitInfo);
			DamageReceiver->ApplyResolvedSniperHit(HitInfo);
		}
	}

	if (USniperWeaponComponent* SniperWeapon = WeaponComponent.Get())
	{
		SniperWeapon->NotifySniperHit(HitInfo);
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
	const float HitSegmentDistance = (Hit.WorldHitLocation - Bullet.PreviousPosition).Length();
	HitInfo.TravelDistance = Bullet.TraveledDistance + HitSegmentDistance;
	HitInfo.ImpactSpeed = Bullet.Velocity.Length();
	HitInfo.RagdollImpulseStrength = Bullet.Damage + HitInfo.ImpactSpeed * 0.1f;
	HitInfo.AmmoType = Bullet.AmmoType;
	HitInfo.HitOutcome = ESniperHitOutcome::Normal;
	HitInfo.bIsScopedShot = Bullet.bWasScopedShot;
	HitInfo.bIsHeadshot = false;
	HitInfo.bIsArmorPiercing = Bullet.bCanDamageArmor;
	HitInfo.bShouldRagdoll = HitInfo.ImpactSpeed >= SniperRagdollImpactSpeedThreshold;
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
