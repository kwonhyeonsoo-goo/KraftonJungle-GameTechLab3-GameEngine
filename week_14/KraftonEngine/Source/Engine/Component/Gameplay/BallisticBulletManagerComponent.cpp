#include "Component/Gameplay/BallisticBulletManagerComponent.h"

#include "Component/Gameplay/CombatCoverAgentComponent.h"
#include "Component/Gameplay/SniperDamageReceiverComponent.h"
#include "Component/Gameplay/SniperWeaponComponent.h"
#include "Component/Primitive/BillboardComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Component/Shape/CapsuleComponent.h"
#include "Core/Logging/Log.h"
#include "GameFramework/Pawn/CombatCharacter.h"
#include "Core/Types/CollisionTypes.h"
#include "Debug/DrawDebugHelpers.h"
#include "GameFramework/AActor.h"
#include "GameFramework/Actor/SniperKillCamDirector.h"
#include "GameFramework/Camera/PlayerCameraManager.h"
#include "GameFramework/GameMode/PlayerController.h"
#include "GameFramework/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialManager.h"
#include "Math/Quat.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Physics/IPhysicsScene.h"
#include "Physics/PhysicsAssetInstance.h"

#include <cmath>
#include <cctype>

#include <algorithm>

namespace
{
	constexpr bool SniperDefaultWindEnabled = true;
	const FVector SniperDefaultWindAcceleration = FVector(0.0f, 1.5f, 0.0f);
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
	constexpr float SniperBallisticSubstepMinDeltaTime = 1.0f / 480.0f;
	constexpr float SniperSpeedOfSoundMetersPerSecond = 343.0f;
	constexpr float SniperBaseDragScale = 0.00008f;

	bool StartsWithToken(const FString& Value, const char* Prefix)
	{
		if (!Prefix)
		{
			return false;
		}

		const size_t PrefixLength = std::char_traits<char>::length(Prefix);
		return Value.size() >= PrefixLength && Value.compare(0, PrefixLength, Prefix) == 0;
	}

	bool IsTokenSeparator(char Character)
	{
		return Character == '_';
	}

	bool HasNormalizedBoneToken(const FString& NormalizedBoneName, const char* Token)
	{
		if (!Token || *Token == '\0' || NormalizedBoneName.empty())
		{
			return false;
		}

		size_t SegmentStart = 0;
		while (SegmentStart < NormalizedBoneName.size())
		{
			size_t SegmentEnd = SegmentStart;
			while (SegmentEnd < NormalizedBoneName.size() && !IsTokenSeparator(NormalizedBoneName[SegmentEnd]))
			{
				++SegmentEnd;
			}

			if (SegmentEnd > SegmentStart)
			{
				const FString Segment = NormalizedBoneName.substr(SegmentStart, SegmentEnd - SegmentStart);
				if (Segment == Token || StartsWithToken(Segment, Token))
				{
					return true;
				}
			}

			SegmentStart = SegmentEnd + 1;
		}

		return false;
	}

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

bool UBallisticBulletManagerComponent::IsWindEnabled() const
{
	const AActor* OwnerActor = GetOwner();
	const UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
	return World ? World->GetWorldSettings().bEnableBallisticWind : SniperDefaultWindEnabled;
}

void UBallisticBulletManagerComponent::SetWindEnabled(bool bInEnableWind)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
	if (World)
	{
		World->GetWorldSettings().bEnableBallisticWind = bInEnableWind;
	}
}

FVector UBallisticBulletManagerComponent::GetWindAcceleration() const
{
	const AActor* OwnerActor = GetOwner();
	const UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
	return World ? World->GetWorldSettings().BallisticWindAcceleration : SniperDefaultWindAcceleration;
}

void UBallisticBulletManagerComponent::SetWindAcceleration(const FVector& InWindAcceleration)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
	if (World)
	{
		World->GetWorldSettings().BallisticWindAcceleration = InWindAcceleration;
	}
}

bool UBallisticBulletManagerComponent::SpawnBullet(const FBallisticBullet& Bullet)
{
	if (!Bullet.bIsAlive)
	{
		return false;
	}

	FBallisticBullet SpawnedBullet = Bullet;
	if (SpawnedBullet.BulletId == 0)
	{
		SpawnedBullet.BulletId = NextBulletId++;
		if (NextBulletId == 0)
		{
			NextBulletId = 1;
		}
	}

	ActiveBullets.push_back(SpawnedBullet);
	const FBulletCinematicSnapshot Snapshot = BuildBulletSnapshot(SpawnedBullet);
	OnBulletSpawned.Broadcast(Snapshot);
	ASniperKillCamDirector::NotifyBulletSpawned(this, Snapshot);
	return true;
}

void UBallisticBulletManagerComponent::ResetBullets()
{
	ActiveBullets.clear();
	HideAllBulletVisuals();
}

bool UBallisticBulletManagerComponent::GetBulletSnapshotById(int32 BulletId, FBulletCinematicSnapshot& OutSnapshot) const
{
	if (BulletId == 0)
	{
		return false;
	}

	for (const FBallisticBullet& Bullet : ActiveBullets)
	{
		if (Bullet.BulletId == BulletId)
		{
			OutSnapshot = BuildBulletSnapshot(Bullet);
			return true;
		}
	}

	return false;
}

FBulletCinematicSnapshot UBallisticBulletManagerComponent::GetLatestBulletSnapshot() const
{
	if (ActiveBullets.empty())
	{
		return FBulletCinematicSnapshot();
	}

	return BuildBulletSnapshot(ActiveBullets.back());
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
	UpdateImpactVisuals(DeltaTime);
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
	const bool bWindEnabled = World ? World->GetWorldSettings().bEnableBallisticWind : SniperDefaultWindEnabled;
	const FVector WorldWindAcceleration = World ? World->GetWorldSettings().BallisticWindAcceleration : SniperDefaultWindAcceleration;
	const FVector AppliedWindAcceleration = bWindEnabled ? WorldWindAcceleration : FVector::ZeroVector;

	if (World)
	{
		DrawWindDebug(World);
	}

	int32 SubstepCount = 1;
	if (bEnableBallisticSubsteps && MaxBallisticSubsteps > 1 && MaxBallisticSubstepDeltaTime > 0.0f)
	{
		SubstepCount = static_cast<int32>(std::ceil(DeltaTime / MaxBallisticSubstepDeltaTime));
		SubstepCount = std::clamp(SubstepCount, 1, MaxBallisticSubsteps);
	}

	const float SubstepDeltaTime = DeltaTime / static_cast<float>(SubstepCount);
	if (SubstepDeltaTime < SniperBallisticSubstepMinDeltaTime)
	{
		SubstepCount = 1;
	}

	for (int32 SubstepIndex = 0; SubstepIndex < SubstepCount; ++SubstepIndex)
	{
		for (FBallisticBullet& Bullet : ActiveBullets)
		{
			UpdateSingleBullet(Bullet, WorldGravity, AppliedWindAcceleration, DeltaTime / static_cast<float>(SubstepCount), World);
		}
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
	if (!World || !World->GetWorldSettings().bEnableBallisticWind)
	{
		return;
	}

	const FVector WindAcceleration = World->GetWorldSettings().BallisticWindAcceleration;
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

void UBallisticBulletManagerComponent::UpdateImpactVisuals(float DeltaTime)
{
	if (ImpactVisualPool.empty())
	{
		return;
	}

	for (int32 VisualIndex = 0; VisualIndex < static_cast<int32>(ImpactVisualPool.size()); ++VisualIndex)
	{
		UBillboardComponent* Visual = ImpactVisualPool[VisualIndex].Get();
		if (!Visual)
		{
			continue;
		}

		if (VisualIndex >= static_cast<int32>(ImpactVisualRemainingTimes.size()))
		{
			Visual->SetVisibility(false);
			continue;
		}

		float& RemainingTime = ImpactVisualRemainingTimes[VisualIndex];
		if (RemainingTime > 0.0f)
		{
			RemainingTime -= DeltaTime;
		}

		if (RemainingTime <= 0.0f)
		{
			RemainingTime = 0.0f;
			Visual->SetVisibility(false);
		}
	}
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
		const float SafeHeadScaleMultiplier = (std::max)(BulletHeadVisualScaleMultiplier, 0.01f);
		const float HeadScale = (std::max)(Bullet.VisualScale * SafeHeadScaleMultiplier, SniperBulletVisualMinScale);
		HeadVisual->SetWorldLocation(Bullet.Position);
		HeadVisual->SetRelativeScale(FVector(1.0f, HeadScale, HeadScale));
		HeadVisual->SetVisibility(Bullet.bIsAlive);

		const FVector Segment = Bullet.Position - Bullet.PreviousPosition;
		const float SegmentDistance = Segment.Length();
		const FVector TracerLocation = Bullet.PreviousPosition + Segment * 0.5f;
		const float SafeTracerWidthMultiplier = (std::max)(BulletTracerWidthMultiplier, 0.01f);
		const float SafeTracerLengthMultiplier = (std::max)(BulletTracerLengthMultiplier, 0.01f);
		const float TracerWidth = (std::max)(Bullet.VisualTracerWidth * SafeTracerWidthMultiplier, SniperBulletTracerMinWidth);
		const float SpeedBasedLength = Bullet.Velocity.Length() * 0.0012f;
		float TracerLength = (std::max)(SegmentDistance, SpeedBasedLength) * Bullet.VisualTracerLengthScale * SafeTracerLengthMultiplier;
		TracerLength = (std::max)(TracerLength, Bullet.VisualTracerMinLength * SafeTracerLengthMultiplier);
		TracerLength = (std::min)(TracerLength, Bullet.VisualTracerMaxLength * SafeTracerLengthMultiplier);
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

	for (TWeakObjectPtr<UBillboardComponent>& VisualEntry : ImpactVisualPool)
	{
		if (UBillboardComponent* Visual = VisualEntry.Get())
		{
			Visual->SetVisibility(false);
		}
	}

	for (float& RemainingTime : ImpactVisualRemainingTimes)
	{
		RemainingTime = 0.0f;
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

UBillboardComponent* UBallisticBulletManagerComponent::GetOrCreateImpactVisual(int32 VisualIndex)
{
	if (VisualIndex < 0)
	{
		return nullptr;
	}

	if (VisualIndex < static_cast<int32>(ImpactVisualPool.size()))
	{
		if (UBillboardComponent* Existing = ImpactVisualPool[VisualIndex].Get())
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

	if (UMaterial* VisualMaterial = ResolveImpactVisualMaterial())
	{
		Visual->SetMaterial(VisualMaterial);
	}

	if (VisualIndex >= static_cast<int32>(ImpactVisualPool.size()))
	{
		ImpactVisualPool.resize(VisualIndex + 1);
	}

	if (VisualIndex >= static_cast<int32>(ImpactVisualRemainingTimes.size()))
	{
		ImpactVisualRemainingTimes.resize(VisualIndex + 1, 0.0f);
	}

	ImpactVisualPool[VisualIndex] = Visual;
	return Visual;
}

UMaterial* UBallisticBulletManagerComponent::ResolveImpactVisualMaterial()
{
	if (UMaterial* Existing = ImpactVisualMaterial.Get())
	{
		return Existing;
	}

	const FString MaterialPath =
		(!ImpactVisualMaterialPath.empty() && ImpactVisualMaterialPath != "None")
		? static_cast<FString>(ImpactVisualMaterialPath)
		: FString(SniperDefaultBulletVisualMaterialPath);

	UMaterial* LoadedMaterial = FMaterialManager::Get().GetOrCreateMaterial(MaterialPath);
	ImpactVisualMaterial = LoadedMaterial;
	return LoadedMaterial;
}

void UBallisticBulletManagerComponent::SpawnImpactVisual(const FVector& ImpactLocation)
{
	if (!bEnableImpactVisuals)
	{
		return;
	}

	int32 FreeVisualIndex = -1;
	for (int32 VisualIndex = 0; VisualIndex < static_cast<int32>(ImpactVisualRemainingTimes.size()); ++VisualIndex)
	{
		if (ImpactVisualRemainingTimes[VisualIndex] <= 0.0f)
		{
			FreeVisualIndex = VisualIndex;
			break;
		}
	}

	if (FreeVisualIndex < 0)
	{
		FreeVisualIndex = static_cast<int32>(ImpactVisualPool.size());
	}

	UBillboardComponent* Visual = GetOrCreateImpactVisual(FreeVisualIndex);
	if (!Visual)
	{
		return;
	}

	if (FreeVisualIndex >= static_cast<int32>(ImpactVisualRemainingTimes.size()))
	{
		ImpactVisualRemainingTimes.resize(FreeVisualIndex + 1, 0.0f);
	}

	ImpactVisualRemainingTimes[FreeVisualIndex] = ImpactVisualLifetime;
	Visual->SetWorldLocation(ImpactLocation);
	Visual->SetRelativeScale(FVector(1.0f, ImpactVisualScale, ImpactVisualScale));
	Visual->SetVisibility(true);
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
			if (!OutHit.bHit)
			{
				return false;
			}

			if (!ShouldRunPreciseCharacterHitQuery(OutHit))
			{
				return true;
			}

			FHitResult PreciseHit;
			if (QueryPreciseCharacterHit(Bullet, World, OutHit, PreciseHit))
			{
				OutHit = PreciseHit;
				return true;
			}

			return !bRequirePreciseCharacterHit;
		}
	}

	if (!World->PhysicsRaycastByObjectTypes(
		Bullet.PreviousPosition,
		Segment / SegmentLength,
		SegmentLength,
		OutHit,
		SniperBulletQueryObjectMask,
		Bullet.Owner))
	{
		return false;
	}

	if (!OutHit.bHit)
	{
		return false;
	}

	if (!ShouldRunPreciseCharacterHitQuery(OutHit))
	{
		return true;
	}

	FHitResult PreciseHit;
	if (QueryPreciseCharacterHit(Bullet, World, OutHit, PreciseHit))
	{
		OutHit = PreciseHit;
		return true;
	}

	return !bRequirePreciseCharacterHit;
}

bool UBallisticBulletManagerComponent::ShouldRunPreciseCharacterHitQuery(const FHitResult& BroadHit) const
{
	if (!bEnablePreciseCharacterHitQuery || !BroadHit.bHit || !BroadHit.HitActor)
	{
		return false;
	}

	USkeletalMeshComponent* SkeletalMeshComponent = ResolveHitSkeletalMeshComponent(BroadHit);
	if (!SkeletalMeshComponent)
	{
		return false;
	}

	if (Cast<UCapsuleComponent>(BroadHit.HitComponent))
	{
		return true;
	}

	if (!Cast<USkeletalMeshComponent>(BroadHit.HitComponent))
	{
		return true;
	}

	return BroadHit.HitBoneName == FName::None;
}

bool UBallisticBulletManagerComponent::EnsurePreciseHitQueryBodies(USkeletalMeshComponent* SkeletalMeshComponent, bool& bOutCreatedTemporaryBodies) const
{
	bOutCreatedTemporaryBodies = false;
	if (!SkeletalMeshComponent)
	{
		return false;
	}

	if (FPhysicsAssetInstance* ExistingInstance = SkeletalMeshComponent->GetPhysicsAssetInstance())
	{
		if (ExistingInstance->HasLivePhysicsObjects())
		{
			return true;
		}
	}

	FPhysicsAssetInstance* PhysicsAssetInstance = SkeletalMeshComponent->GetOrCreatePhysicsAssetInstance();
	if (!PhysicsAssetInstance)
	{
		return false;
	}

	FPhysicsAssetSimulationOptions QueryOnlyOptions;
	QueryOnlyOptions.bNoGravity = true;
	QueryOnlyOptions.bCreateKinematicQueryOnlyBodies = true;
	QueryOnlyOptions.bUseIndependentRagdollCollision = true;
	QueryOnlyOptions.IndependentCollisionEnabled = ECollisionEnabled::QueryOnly;
	QueryOnlyOptions.bIndependentGenerateOverlapEvents = false;

	if (!PhysicsAssetInstance->CreateBodiesAndConstraints(QueryOnlyOptions))
	{
		return false;
	}

	bOutCreatedTemporaryBodies = true;
	return PhysicsAssetInstance->HasLivePhysicsObjects();
}

bool UBallisticBulletManagerComponent::QueryPreciseCharacterHit(
	const FBallisticBullet& Bullet,
	UWorld* World,
	const FHitResult& BroadHit,
	FHitResult& OutPreciseHit) const
{
	OutPreciseHit = FHitResult();
	if (!World || !BroadHit.HitActor)
	{
		return false;
	}

	USkeletalMeshComponent* SkeletalMeshComponent = ResolveHitSkeletalMeshComponent(BroadHit);
	if (!SkeletalMeshComponent)
	{
		return false;
	}

	IPhysicsScene* PhysicsScene = World->GetPhysicsScene();
	if (!PhysicsScene)
	{
		return false;
	}

	bool bCreatedTemporaryBodies = false;
	if (!EnsurePreciseHitQueryBodies(SkeletalMeshComponent, bCreatedTemporaryBodies))
	{
		return false;
	}

	const auto CleanupTemporaryBodies = [&]()
	{
		if (bCreatedTemporaryBodies)
		{
			if (FPhysicsAssetInstance* PhysicsAssetInstance = SkeletalMeshComponent->GetPhysicsAssetInstance())
			{
				PhysicsAssetInstance->DestroyBodiesAndConstraints();
			}
		}
	};

	bool bPreciseHit = false;
	if (Bullet.Radius > SniperBulletMinSweepRadius)
	{
		const FCollisionShape SweepShape = FCollisionShape::MakeSphere(Bullet.Radius);
		bPreciseHit = PhysicsScene->SweepRagdollBodiesByObjectTypes(
			Bullet.PreviousPosition,
			Bullet.Position,
			FQuat::Identity,
			SweepShape,
			OutPreciseHit,
			ObjectTypeBit(ECollisionChannel::Pawn),
			BroadHit.HitActor,
			Bullet.Owner);
	}
	else
	{
		const FVector Segment = Bullet.Position - Bullet.PreviousPosition;
		const float SegmentLength = Segment.Length();
		if (SegmentLength > SniperBulletMinSweepRadius)
		{
			bPreciseHit = PhysicsScene->RaycastRagdollBodiesByObjectTypes(
				Bullet.PreviousPosition,
				Segment / SegmentLength,
				SegmentLength,
				OutPreciseHit,
				ObjectTypeBit(ECollisionChannel::Pawn),
				BroadHit.HitActor,
				Bullet.Owner);
		}
	}

	if (bPreciseHit && MaxPreciseCharacterHitDistance > 0.0f)
	{
		const float MaxDistanceSquared = MaxPreciseCharacterHitDistance * MaxPreciseCharacterHitDistance;
		if (FVector::DistSquared(BroadHit.WorldHitLocation, OutPreciseHit.WorldHitLocation) > MaxDistanceSquared)
		{
			bPreciseHit = false;
			OutPreciseHit = FHitResult();
		}
	}

	CleanupTemporaryBodies();
	return bPreciseHit;
}

void UBallisticBulletManagerComponent::HandleBulletHit(FBallisticBullet& Bullet, const FHitResult& Hit, UWorld* World)
{
	Bullet.Position = Hit.WorldHitLocation;
	Bullet.bIsAlive = false;

	SpawnImpactVisual(Hit.WorldHitLocation);

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
		float HealthBeforeHit = -1.0f;
		float HealthAfterHit = -1.0f;
		const FString RawHitBoneName = Hit.HitBoneName.ToString();
		const FString ResolvedHitBoneName = HitInfo.HitBoneName.ToString();
		const FString NormalizedHitBoneName = NormalizeBoneNameForHitClassification(HitInfo.HitBoneName);
		const bool bUsedFallbackBone =
			Hit.HitBoneName == FName::None || Hit.HitBoneName != HitInfo.HitBoneName;
		if (USniperDamageReceiverComponent* DamageReceiver = HitActor->GetComponentByClass<USniperDamageReceiverComponent>())
		{
			HealthBeforeHit = DamageReceiver->GetCurrentHP();
			HitInfo = DamageReceiver->ResolveSniperHit(HitInfo);
			DamageReceiver->ApplyResolvedSniperHit(HitInfo);
			HealthAfterHit = DamageReceiver->GetCurrentHP();
		}

		if (ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(HitActor))
		{
			UCombatCoverAgentComponent* CombatAgent = CombatCharacter->GetCombatCoverAgentComponent();
			const float FallbackCurrentHealth = CombatAgent ? CombatAgent->GetHealth() : -1.0f;
			if (HealthBeforeHit < 0.0f)
			{
				HealthBeforeHit = FallbackCurrentHealth;
			}
			if (HealthAfterHit < 0.0f)
			{
				HealthAfterHit = FallbackCurrentHealth;
			}

			UE_LOG(
				"[SniperDebug] Bullet hit CombatCharacter: Actor=%s Team=%s HealthBefore=%.1f HealthAfter=%.1f Damage=%.1f RegionMultiplier=%.2f Outcome=%d Region=%d Killed=%d Headshot=%d RawHitBone=%s HitBone=%s NormalizedHitBone=%s UsedFallbackBone=%d",
				CombatCharacter->GetName().c_str(),
				CombatAgent ? CombatAgent->GetTeamTag().c_str() : "Unknown",
				HealthBeforeHit,
				HealthAfterHit,
				HitInfo.Damage,
				HitInfo.RegionDamageMultiplier,
				static_cast<int32>(HitInfo.HitOutcome),
				static_cast<int32>(HitInfo.HitRegion),
				HealthBeforeHit > 0.0f && HealthAfterHit <= 0.0f ? 1 : 0,
				HitInfo.bIsHeadshot ? 1 : 0,
				RawHitBoneName.c_str(),
				ResolvedHitBoneName.c_str(),
				NormalizedHitBoneName.c_str(),
				bUsedFallbackBone ? 1 : 0);
		}
	}

	if (USniperWeaponComponent* SniperWeapon = WeaponComponent.Get())
	{
		SniperWeapon->NotifySniperHit(HitInfo);
	}

	ASniperKillCamDirector::NotifyBulletHit(HitInfo);
}

FSniperHitInfo UBallisticBulletManagerComponent::BuildSniperHitInfo(const FBallisticBullet& Bullet, const FHitResult& Hit) const
{
	FSniperHitInfo HitInfo;
	const FName ResolvedHitBoneName = ResolvePreciseHitBoneName(Hit);
	const ESniperHitRegion HitRegion = ClassifyHitRegion(ResolvedHitBoneName);
	HitInfo.BulletId = Bullet.BulletId;
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
	HitInfo.HitRegion = HitRegion;
	HitInfo.bIsScopedShot = Bullet.bWasScopedShot;
	HitInfo.bIsHeadshot = HitRegion == ESniperHitRegion::Head;
	HitInfo.bIsArmorPiercing = Bullet.bCanDamageArmor;
	HitInfo.bShouldRagdoll = HitInfo.ImpactSpeed >= SniperRagdollImpactSpeedThreshold;
	HitInfo.bKilled = false;
	HitInfo.bFriendlyTarget = false;
	HitInfo.Shooter = Bullet.Owner;
	HitInfo.RegionDamageMultiplier = 1.0f;
	HitInfo.TargetCurrentHP = 0.0f;
	HitInfo.TargetMaxHP = 0.0f;
	HitInfo.HitBoneName = ResolvedHitBoneName;
	return HitInfo;
}

USkeletalMeshComponent* UBallisticBulletManagerComponent::ResolveHitSkeletalMeshComponent(const FHitResult& Hit) const
{
	if (USkeletalMeshComponent* HitSkeletalMesh = Cast<USkeletalMeshComponent>(Hit.HitComponent))
	{
		return HitSkeletalMesh;
	}

	if (AActor* HitActor = Hit.HitActor)
	{
		return HitActor->GetComponentByClass<USkeletalMeshComponent>();
	}

	return nullptr;
}

FName UBallisticBulletManagerComponent::ResolvePreciseHitBoneName(const FHitResult& Hit, bool* bOutUsedFallback) const
{
	if (bOutUsedFallback)
	{
		*bOutUsedFallback = false;
	}

	if (Hit.HitBoneName.IsValid() && Hit.HitBoneName != FName::None)
	{
		const FString NormalizedRawBoneName = NormalizeBoneNameForHitClassification(Hit.HitBoneName);
		if (!NormalizedRawBoneName.empty() && !IsAuxiliaryBoneNameNormalized(NormalizedRawBoneName))
		{
			return Hit.HitBoneName;
		}
	}

	USkeletalMeshComponent* SkeletalMeshComponent = ResolveHitSkeletalMeshComponent(Hit);
	if (!SkeletalMeshComponent)
	{
		return FName::None;
	}

	if (FPhysicsAssetInstance* PhysicsAssetInstance = SkeletalMeshComponent->GetPhysicsAssetInstance())
	{
		FName NearestBodyBoneName = FName::None;
		FVector NearestBodyLocation = FVector::ZeroVector;
		if (PhysicsAssetInstance->FindNearestBodyToWorldLocation(
				Hit.WorldHitLocation,
				NearestBodyBoneName,
				NearestBodyLocation))
		{
			const FString NormalizedNearestBodyBoneName = NormalizeBoneNameForHitClassification(NearestBodyBoneName);
			if (!NormalizedNearestBodyBoneName.empty() && !IsAuxiliaryBoneNameNormalized(NormalizedNearestBodyBoneName))
			{
				if (bOutUsedFallback)
				{
					*bOutUsedFallback = true;
				}
				return NearestBodyBoneName;
			}
		}
	}

	USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMesh();
	FSkeletalMesh* MeshAsset = SkeletalMesh ? SkeletalMesh->GetSkeletalMeshAsset() : nullptr;
	if (!MeshAsset || MeshAsset->Bones.empty())
	{
		return FName::None;
	}

	float BestDistanceSquared = 0.0f;
	int32 BestBoneIndex = -1;
	for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++BoneIndex)
	{
		const FString& BoneName = MeshAsset->Bones[BoneIndex].Name;
		if (BoneName.empty())
		{
			continue;
		}

		const FString NormalizedBoneName = NormalizeBoneNameForHitClassification(FName(BoneName));
		if (NormalizedBoneName.empty() || IsAuxiliaryBoneNameNormalized(NormalizedBoneName))
		{
			continue;
		}

		const FVector BoneLocation = SkeletalMeshComponent->GetBoneLocationByIndex(BoneIndex);
		const float DistanceSquared = FVector::DistSquared(Hit.WorldHitLocation, BoneLocation);
		if (BestBoneIndex < 0 || DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestBoneIndex = BoneIndex;
		}
	}

	if (BestBoneIndex < 0 || BestBoneIndex >= static_cast<int32>(MeshAsset->Bones.size()))
	{
		return FName::None;
	}

	if (bOutUsedFallback)
	{
		*bOutUsedFallback = true;
	}
	return FName(MeshAsset->Bones[BestBoneIndex].Name);
}

FString UBallisticBulletManagerComponent::NormalizeBoneNameForHitClassification(const FName& BoneName) const
{
	if (!BoneName.IsValid() || BoneName == FName::None)
	{
		return FString();
	}

	const FString RawBoneName = BoneName.ToString();
	FString NormalizedBoneName;
	NormalizedBoneName.reserve(RawBoneName.size());

	bool bLastCharacterWasSeparator = false;
	for (char Character : RawBoneName)
	{
		const unsigned char UnsignedCharacter = static_cast<unsigned char>(Character);
		if (std::isalnum(UnsignedCharacter) != 0)
		{
			NormalizedBoneName.push_back(static_cast<char>(std::tolower(UnsignedCharacter)));
			bLastCharacterWasSeparator = false;
		}
		else if (!bLastCharacterWasSeparator && !NormalizedBoneName.empty())
		{
			NormalizedBoneName.push_back('_');
			bLastCharacterWasSeparator = true;
		}
	}

	while (!NormalizedBoneName.empty() && NormalizedBoneName.back() == '_')
	{
		NormalizedBoneName.pop_back();
	}

	return NormalizedBoneName;
}

bool UBallisticBulletManagerComponent::IsAuxiliaryBoneNameNormalized(const FString& BoneName) const
{
	return HasNormalizedBoneToken(BoneName, "ik") ||
		HasNormalizedBoneToken(BoneName, "weapon") ||
		HasNormalizedBoneToken(BoneName, "camera") ||
		HasNormalizedBoneToken(BoneName, "twist") ||
		HasNormalizedBoneToken(BoneName, "socket") ||
		HasNormalizedBoneToken(BoneName, "ctrl") ||
		HasNormalizedBoneToken(BoneName, "control") ||
		HasNormalizedBoneToken(BoneName, "target") ||
		HasNormalizedBoneToken(BoneName, "pole") ||
		HasNormalizedBoneToken(BoneName, "end") ||
		HasNormalizedBoneToken(BoneName, "nub") ||
		HasNormalizedBoneToken(BoneName, "offset") ||
		HasNormalizedBoneToken(BoneName, "attach") ||
		HasNormalizedBoneToken(BoneName, "helper");
}

ESniperHitRegion UBallisticBulletManagerComponent::ClassifyHitRegionNormalized(const FString& BoneName) const
{
	if (BoneName.empty())
	{
		return ESniperHitRegion::Unknown;
	}

	if (IsHeadshotBoneNameNormalized(BoneName))
	{
		return ESniperHitRegion::Head;
	}

	if (HasNormalizedBoneToken(BoneName, "spine") ||
		HasNormalizedBoneToken(BoneName, "pelvis") ||
		HasNormalizedBoneToken(BoneName, "hips") ||
		HasNormalizedBoneToken(BoneName, "hip") ||
		HasNormalizedBoneToken(BoneName, "chest") ||
		HasNormalizedBoneToken(BoneName, "upperchest") ||
		HasNormalizedBoneToken(BoneName, "torso") ||
		HasNormalizedBoneToken(BoneName, "rib") ||
		HasNormalizedBoneToken(BoneName, "clavicle") ||
		HasNormalizedBoneToken(BoneName, "neck"))
	{
		return ESniperHitRegion::Torso;
	}

	if (HasNormalizedBoneToken(BoneName, "arm") ||
		HasNormalizedBoneToken(BoneName, "shoulder") ||
		HasNormalizedBoneToken(BoneName, "elbow") ||
		HasNormalizedBoneToken(BoneName, "forearm") ||
		HasNormalizedBoneToken(BoneName, "hand") ||
		HasNormalizedBoneToken(BoneName, "wrist"))
	{
		return ESniperHitRegion::Arm;
	}

	if (HasNormalizedBoneToken(BoneName, "leg") ||
		HasNormalizedBoneToken(BoneName, "thigh") ||
		HasNormalizedBoneToken(BoneName, "calf") ||
		HasNormalizedBoneToken(BoneName, "knee") ||
		HasNormalizedBoneToken(BoneName, "foot") ||
		HasNormalizedBoneToken(BoneName, "ankle") ||
		HasNormalizedBoneToken(BoneName, "toe"))
	{
		return ESniperHitRegion::Leg;
	}

	return ESniperHitRegion::Unknown;
}

ESniperHitRegion UBallisticBulletManagerComponent::ClassifyHitRegion(const FName& BoneName) const
{
	return ClassifyHitRegionNormalized(NormalizeBoneNameForHitClassification(BoneName));
}

bool UBallisticBulletManagerComponent::IsHeadshotBoneNameNormalized(const FString& BoneName) const
{
	return HasNormalizedBoneToken(BoneName, "head") ||
		HasNormalizedBoneToken(BoneName, "skull") ||
		HasNormalizedBoneToken(BoneName, "face") ||
		HasNormalizedBoneToken(BoneName, "jaw") ||
		HasNormalizedBoneToken(BoneName, "eye");
}

bool UBallisticBulletManagerComponent::IsHeadshotBoneName(const FName& BoneName) const
{
	return IsHeadshotBoneNameNormalized(NormalizeBoneNameForHitClassification(BoneName));
}

FBulletCinematicSnapshot UBallisticBulletManagerComponent::BuildBulletSnapshot(const FBallisticBullet& Bullet) const
{
	FBulletCinematicSnapshot Snapshot;
	Snapshot.BulletId = Bullet.BulletId;
	Snapshot.Position = Bullet.Position;
	Snapshot.PreviousPosition = Bullet.PreviousPosition;
	Snapshot.Velocity = Bullet.Velocity;
	Snapshot.TraveledDistance = Bullet.TraveledDistance;
	Snapshot.LifeTime = Bullet.LifeTime;
	Snapshot.AmmoType = Bullet.AmmoType;
	Snapshot.Owner = IsValid(Bullet.Owner) ? Bullet.Owner : nullptr;
	Snapshot.bIsAlive = Bullet.bIsAlive;
	Snapshot.bWasScopedShot = Bullet.bWasScopedShot;
	return Snapshot;
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
