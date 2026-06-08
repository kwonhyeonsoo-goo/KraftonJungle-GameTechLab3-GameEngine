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
#include "Physics/PhysicsAsset.h"
#include "Physics/PhysicsAssetInstance.h"
#include "Physics/PhysicsAssetPreviewUtils.h"

#include <cmath>
#include <cctype>

#include <algorithm>
#include <cfloat>

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
	constexpr float SniperRagdollImpactSpeedThreshold = 300.0f;
	constexpr const char* SniperDefaultBulletVisualMaterialPath = "Content/Material/Particle/ParticleSprite.uasset";
	constexpr float SniperBulletVisualMinScale = 0.04f;
	constexpr float SniperBulletTracerMinWidth = 0.01f;
	constexpr float SniperBulletTracerDefaultThickness = 1.0f;
	constexpr float SniperBallisticSubstepMinDeltaTime = 1.0f / 480.0f;
	constexpr float SniperSpeedOfSoundMetersPerSecond = 343.0f;
	constexpr float SniperBaseDragScale = 0.00008f;
	constexpr float SniperPhysicsAssetHitMinShapeSize = 0.001f;
	constexpr float SniperPhysicsAssetHitEpsilon = 1.0e-6f;

	struct FSniperPoseShapeHit
	{
		bool bHit = false;
		float T = FLT_MAX;
		FName BoneName = FName::None;
		FVector WorldNormal = FVector::ZeroVector;
		int32 BodyIndex = -1;
		int32 ShapeIndex = -1;
	};

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

	FTransform ComposeSniperPhysicsAssetTransforms(const FTransform& ParentWorld, const FTransform& Local)
	{
		FTransform Result = Local;
		Result.Location = ParentWorld.Location + ParentWorld.Rotation.RotateVector(Local.Location);
		Result.Rotation = (ParentWorld.Rotation * Local.Rotation).GetNormalized();
		Result.Scale = FVector::OneVector;
		return Result;
	}

	FVector TransformWorldPositionToShapeLocal(const FVector& WorldPosition, const FTransform& ShapeWorld)
	{
		const FQuat InverseRotation = ShapeWorld.Rotation.GetNormalized().Inverse();
		return InverseRotation.RotateVector(WorldPosition - ShapeWorld.Location);
	}

	float GetAxisValue(const FVector& Value, int32 Axis)
	{
		return Value.Data[Axis];
	}

	float ClampUnit(float Value)
	{
		return FMath::Clamp(Value, 0.0f, 1.0f);
	}

	FVector ComputeBoxNormalLocal(const FVector& LocalPoint, const FVector& HalfExtent)
	{
		const float DistToX = std::abs(HalfExtent.X - std::abs(LocalPoint.X));
		const float DistToY = std::abs(HalfExtent.Y - std::abs(LocalPoint.Y));
		const float DistToZ = std::abs(HalfExtent.Z - std::abs(LocalPoint.Z));
		if (DistToX <= DistToY && DistToX <= DistToZ)
		{
			return FVector(LocalPoint.X >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
		}
		if (DistToY <= DistToZ)
		{
			return FVector(0.0f, LocalPoint.Y >= 0.0f ? 1.0f : -1.0f, 0.0f);
		}
		return FVector(0.0f, 0.0f, LocalPoint.Z >= 0.0f ? 1.0f : -1.0f);
	}

	bool IntersectSegmentLocalBox(const FVector& Start, const FVector& End, const FVector& HalfExtent, float& OutT)
	{
		float TMin = 0.0f;
		float TMax = 1.0f;
		const FVector Delta = End - Start;
		const float MinBounds[3] = { -HalfExtent.X, -HalfExtent.Y, -HalfExtent.Z };
		const float MaxBounds[3] = {  HalfExtent.X,  HalfExtent.Y,  HalfExtent.Z };

		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			const float Origin = GetAxisValue(Start, Axis);
			const float Direction = GetAxisValue(Delta, Axis);
			if (std::abs(Direction) < SniperPhysicsAssetHitEpsilon)
			{
				if (Origin < MinBounds[Axis] || Origin > MaxBounds[Axis])
				{
					return false;
				}
				continue;
			}

			float T1 = (MinBounds[Axis] - Origin) / Direction;
			float T2 = (MaxBounds[Axis] - Origin) / Direction;
			if (T1 > T2)
			{
				std::swap(T1, T2);
			}

			TMin = (std::max)(TMin, T1);
			TMax = (std::min)(TMax, T2);
			if (TMin > TMax)
			{
				return false;
			}
		}

		OutT = ClampUnit(TMin);
		return true;
	}

	bool IntersectSegmentLocalSphere(const FVector& Start, const FVector& End, float Radius, float& OutT)
	{
		const FVector Delta = End - Start;
		const float A = Delta.Dot(Delta);
		if (A < SniperPhysicsAssetHitEpsilon)
		{
			if (Start.Dot(Start) <= Radius * Radius)
			{
				OutT = 0.0f;
				return true;
			}
			return false;
		}

		const float B = 2.0f * Start.Dot(Delta);
		const float C = Start.Dot(Start) - Radius * Radius;
		const float Discriminant = B * B - 4.0f * A * C;
		if (Discriminant < 0.0f)
		{
			return false;
		}

		const float SqrtDisc = sqrtf(Discriminant);
		const float InvDenominator = 1.0f / (2.0f * A);
		const float Candidates[2] = {
			(-B - SqrtDisc) * InvDenominator,
			(-B + SqrtDisc) * InvDenominator
		};

		float BestT = FLT_MAX;
		for (float T : Candidates)
		{
			if (T >= 0.0f && T <= 1.0f)
			{
				BestT = (std::min)(BestT, T);
			}
		}

		if (BestT == FLT_MAX)
		{
			return false;
		}

		OutT = BestT;
		return true;
	}

	bool IntersectSegmentLocalCylinderZ(const FVector& Start, const FVector& End, float Radius, float CylinderHalfHeight, float& OutT)
	{
		if (CylinderHalfHeight <= 0.0f)
		{
			return false;
		}

		const FVector Delta = End - Start;
		const float A = Delta.X * Delta.X + Delta.Y * Delta.Y;
		if (A < SniperPhysicsAssetHitEpsilon)
		{
			return false;
		}

		const float B = 2.0f * (Start.X * Delta.X + Start.Y * Delta.Y);
		const float C = Start.X * Start.X + Start.Y * Start.Y - Radius * Radius;
		const float Discriminant = B * B - 4.0f * A * C;
		if (Discriminant < 0.0f)
		{
			return false;
		}

		const float SqrtDisc = sqrtf(Discriminant);
		const float InvDenominator = 1.0f / (2.0f * A);
		const float Candidates[2] = {
			(-B - SqrtDisc) * InvDenominator,
			(-B + SqrtDisc) * InvDenominator
		};

		float BestT = FLT_MAX;
		for (float T : Candidates)
		{
			if (T < 0.0f || T > 1.0f)
			{
				continue;
			}

			const float Z = Start.Z + Delta.Z * T;
			if (Z >= -CylinderHalfHeight && Z <= CylinderHalfHeight)
			{
				BestT = (std::min)(BestT, T);
			}
		}

		if (BestT == FLT_MAX)
		{
			return false;
		}

		OutT = BestT;
		return true;
	}

	bool IntersectSegmentLocalCapsuleZ(const FVector& Start, const FVector& End, float Radius, float HalfHeight, float& OutT)
	{
		const float SafeRadius = (std::max)(Radius, SniperPhysicsAssetHitMinShapeSize);
		const float SafeHalfHeight = (std::max)(HalfHeight, SafeRadius);
		const float CylinderHalfHeight = (std::max)(0.0f, SafeHalfHeight - SafeRadius);

		float BestT = FLT_MAX;
		float T = 0.0f;
		if (IntersectSegmentLocalCylinderZ(Start, End, SafeRadius, CylinderHalfHeight, T))
		{
			BestT = (std::min)(BestT, T);
		}

		if (IntersectSegmentLocalSphere(Start - FVector(0.0f, 0.0f, CylinderHalfHeight), End - FVector(0.0f, 0.0f, CylinderHalfHeight), SafeRadius, T))
		{
			BestT = (std::min)(BestT, T);
		}

		if (IntersectSegmentLocalSphere(Start - FVector(0.0f, 0.0f, -CylinderHalfHeight), End - FVector(0.0f, 0.0f, -CylinderHalfHeight), SafeRadius, T))
		{
			BestT = (std::min)(BestT, T);
		}

		if (BestT == FLT_MAX)
		{
			return false;
		}

		OutT = BestT;
		return true;
	}

	FVector ComputeCapsuleNormalLocal(const FVector& LocalPoint, float Radius, float HalfHeight)
	{
		const float SafeRadius = (std::max)(Radius, SniperPhysicsAssetHitMinShapeSize);
		const float SafeHalfHeight = (std::max)(HalfHeight, SafeRadius);
		const float CylinderHalfHeight = (std::max)(0.0f, SafeHalfHeight - SafeRadius);
		const float ClampedZ = FMath::Clamp(LocalPoint.Z, -CylinderHalfHeight, CylinderHalfHeight);
		FVector Normal = LocalPoint - FVector(0.0f, 0.0f, ClampedZ);
		if (Normal.IsNearlyZero())
		{
			Normal = LocalPoint.Z >= 0.0f ? FVector::UpVector : FVector::DownVector;
		}
		else
		{
			Normal.Normalize();
		}
		return Normal;
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
	return World ? World->GetCurrentBallisticWindAcceleration() : SniperDefaultWindAcceleration;
}

void UBallisticBulletManagerComponent::SetWindAcceleration(const FVector& InWindAcceleration)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
	if (World)
	{
		World->SetBallisticWindAcceleration(InWindAcceleration);
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
	const FVector WorldWindAcceleration = World ? World->GetCurrentBallisticWindAcceleration() : SniperDefaultWindAcceleration;
	const FVector AppliedWindAcceleration = bWindEnabled ? WorldWindAcceleration : FVector::ZeroVector;

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

			FHitResult PreciseHit;
			if (ShouldRunPreciseCharacterHitQuery(OutHit))
			{
				if (QueryPreciseCharacterHit(Bullet, World, OutHit, PreciseHit))
				{
					OutHit = PreciseHit;
					return true;
				}
			}

			if (ShouldRunPosePhysicsAssetHitQuery(OutHit))
			{
				if (QueryPosePhysicsAssetCharacterHit(Bullet, OutHit, PreciseHit))
				{
					OutHit = PreciseHit;
					return true;
				}

				UE_LOG(
					"[SniperDebug] Character broad hit rejected by PhysicsAsset precision. Actor=%s Component=%s Bone=%s",
					OutHit.HitActor ? OutHit.HitActor->GetName().c_str() : "None",
					OutHit.HitComponent ? OutHit.HitComponent->GetName().c_str() : "None",
					OutHit.HitBoneName.ToString().c_str());
				return false;
			}

			return true;
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

	FHitResult PreciseHit;
	if (ShouldRunPreciseCharacterHitQuery(OutHit))
	{
		if (QueryPreciseCharacterHit(Bullet, World, OutHit, PreciseHit))
		{
			OutHit = PreciseHit;
			return true;
		}
	}

	if (ShouldRunPosePhysicsAssetHitQuery(OutHit))
	{
		if (QueryPosePhysicsAssetCharacterHit(Bullet, OutHit, PreciseHit))
		{
			OutHit = PreciseHit;
			return true;
		}

		UE_LOG(
			"[SniperDebug] Character broad hit rejected by PhysicsAsset precision. Actor=%s Component=%s Bone=%s",
			OutHit.HitActor ? OutHit.HitActor->GetName().c_str() : "None",
			OutHit.HitComponent ? OutHit.HitComponent->GetName().c_str() : "None",
			OutHit.HitBoneName.ToString().c_str());
		return false;
	}

	return true;
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

	FPhysicsAssetInstance* PhysicsAssetInstance = SkeletalMeshComponent->GetPhysicsAssetInstance();
	if (!PhysicsAssetInstance || !PhysicsAssetInstance->HasLivePhysicsObjects())
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

bool UBallisticBulletManagerComponent::ShouldRunPosePhysicsAssetHitQuery(const FHitResult& BroadHit) const
{
	if (!bEnablePreciseCharacterHitQuery || !BroadHit.bHit || !BroadHit.HitActor)
	{
		return false;
	}

	if (!Cast<ACombatCharacter>(BroadHit.HitActor) &&
		!BroadHit.HitActor->GetComponentByClass<USniperDamageReceiverComponent>())
	{
		return false;
	}

	USkeletalMeshComponent* SkeletalMeshComponent = ResolveHitSkeletalMeshComponent(BroadHit);
	if (!SkeletalMeshComponent)
	{
		return false;
	}

	UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetEffectivePhysicsAsset();
	if (!PhysicsAsset || PhysicsAsset->GetBodySetups().empty())
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

	// Do not auto-create temporary PhysicsAsset bodies for precise hit queries.
	// If the character is not already running with live bodies, we skip the precise pass
	// and keep the normal broad-hit path so gameplay actors never enter a transient
	// physics-pose state during ordinary PIE startup or hits.
	if (FPhysicsAssetInstance* ExistingInstance = SkeletalMeshComponent->GetPhysicsAssetInstance())
	{
		return ExistingInstance->HasLivePhysicsObjects();
	}

	return false;
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

bool UBallisticBulletManagerComponent::QueryPosePhysicsAssetCharacterHit(
	const FBallisticBullet& Bullet,
	const FHitResult& BroadHit,
	FHitResult& OutPreciseHit) const
{
	OutPreciseHit = FHitResult();
	if (!BroadHit.bHit || !BroadHit.HitActor)
	{
		return false;
	}

	USkeletalMeshComponent* SkeletalMeshComponent = ResolveHitSkeletalMeshComponent(BroadHit);
	if (!SkeletalMeshComponent)
	{
		return false;
	}

	UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetEffectivePhysicsAsset();
	if (!PhysicsAsset)
	{
		return false;
	}

	const FVector Segment = Bullet.Position - Bullet.PreviousPosition;
	const float SegmentLength = Segment.Length();
	if (SegmentLength <= SniperBulletMinSweepRadius)
	{
		return false;
	}

	FPhysicsAssetPreviewPoseCache PoseCache;
	if (!PoseCache.Initialize(SkeletalMeshComponent, PhysicsAsset))
	{
		UE_LOG(
			"[SniperDebug] PhysicsAsset precision skipped: pose cache unavailable. Actor=%s Mesh=%s",
			BroadHit.HitActor ? BroadHit.HitActor->GetName().c_str() : "None",
			SkeletalMeshComponent->GetName().c_str());
		return false;
	}

	const float BulletRadius = (std::max)(Bullet.Radius, 0.0f);
	const TArray<FPhysicsAssetBodySetup>& BodySetups = PhysicsAsset->GetBodySetups();
	FSniperPoseShapeHit BestHit;

	for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(BodySetups.size()); ++BodyIndex)
	{
		const FPhysicsAssetBodySetup& BodySetup = BodySetups[BodyIndex];
		if (!BodySetup.BoneName.IsValid() || BodySetup.BoneName == FName::None || BodySetup.Shapes.empty())
		{
			continue;
		}

		const FString NormalizedBodyBoneName = NormalizeBoneNameForHitClassification(BodySetup.BoneName);
		if (NormalizedBodyBoneName.empty() || IsAuxiliaryBoneNameNormalized(NormalizedBodyBoneName))
		{
			continue;
		}

		FTransform BodyWorld;
		if (!PoseCache.ComputeBodyWorldTransform(BodyIndex, BodyWorld))
		{
			continue;
		}

		for (int32 ShapeIndex = 0; ShapeIndex < static_cast<int32>(BodySetup.Shapes.size()); ++ShapeIndex)
		{
			const FPhysicsAssetShapeSetup& ShapeSetup = BodySetup.Shapes[ShapeIndex];
			const FTransform ShapeWorld = ComposeSniperPhysicsAssetTransforms(BodyWorld, ShapeSetup.LocalTransform);
			const FVector LocalStart = TransformWorldPositionToShapeLocal(Bullet.PreviousPosition, ShapeWorld);
			const FVector LocalEnd = TransformWorldPositionToShapeLocal(Bullet.Position, ShapeWorld);

			float T = 0.0f;
			bool bShapeHit = false;
			FVector LocalNormal = FVector::ZeroVector;
			switch (ShapeSetup.Type)
			{
			case EPhysicsAssetShapeType::Box:
			{
				const FVector HalfExtent(
					(std::max)(ShapeSetup.BoxHalfExtent.X, SniperPhysicsAssetHitMinShapeSize) + BulletRadius,
					(std::max)(ShapeSetup.BoxHalfExtent.Y, SniperPhysicsAssetHitMinShapeSize) + BulletRadius,
					(std::max)(ShapeSetup.BoxHalfExtent.Z, SniperPhysicsAssetHitMinShapeSize) + BulletRadius);
				bShapeHit = IntersectSegmentLocalBox(LocalStart, LocalEnd, HalfExtent, T);
				if (bShapeHit)
				{
					LocalNormal = ComputeBoxNormalLocal(FVector::Lerp(LocalStart, LocalEnd, T), HalfExtent);
				}
				break;
			}
			case EPhysicsAssetShapeType::Sphere:
			{
				const float Radius = (std::max)(ShapeSetup.SphereRadius, SniperPhysicsAssetHitMinShapeSize) + BulletRadius;
				bShapeHit = IntersectSegmentLocalSphere(LocalStart, LocalEnd, Radius, T);
				if (bShapeHit)
				{
					LocalNormal = FVector::Lerp(LocalStart, LocalEnd, T);
					if (LocalNormal.IsNearlyZero())
					{
						LocalNormal = FVector::ForwardVector;
					}
					else
					{
						LocalNormal.Normalize();
					}
				}
				break;
			}
			case EPhysicsAssetShapeType::Capsule:
			{
				const float ShapeRadius = (std::max)(ShapeSetup.CapsuleRadius, SniperPhysicsAssetHitMinShapeSize);
				const float ShapeHalfHeight = (std::max)(ShapeSetup.CapsuleHalfHeight, ShapeRadius);
				const float Radius = ShapeRadius + BulletRadius;
				const float HalfHeight = ShapeHalfHeight + BulletRadius;
				bShapeHit = IntersectSegmentLocalCapsuleZ(LocalStart, LocalEnd, Radius, HalfHeight, T);
				if (bShapeHit)
				{
					LocalNormal = ComputeCapsuleNormalLocal(FVector::Lerp(LocalStart, LocalEnd, T), Radius, HalfHeight);
				}
				break;
			}
			default:
				break;
			}

			if (!bShapeHit || T < 0.0f || T > 1.0f || T >= BestHit.T)
			{
				continue;
			}

			BestHit.bHit = true;
			BestHit.T = T;
			BestHit.BoneName = BodySetup.BoneName;
			BestHit.WorldNormal = ShapeWorld.Rotation.GetNormalized().RotateVector(LocalNormal);
			if (!BestHit.WorldNormal.IsNearlyZero())
			{
				BestHit.WorldNormal.Normalize();
			}
			BestHit.BodyIndex = BodyIndex;
			BestHit.ShapeIndex = ShapeIndex;
		}
	}

	if (!BestHit.bHit)
	{
		return false;
	}

	OutPreciseHit = BroadHit;
	OutPreciseHit.bHit = true;
	OutPreciseHit.HitActor = BroadHit.HitActor;
	OutPreciseHit.HitComponent = SkeletalMeshComponent;
	OutPreciseHit.HitBoneName = BestHit.BoneName;
	OutPreciseHit.WorldHitLocation = Bullet.PreviousPosition + Segment * BestHit.T;
	OutPreciseHit.Distance = SegmentLength * BestHit.T;
	OutPreciseHit.WorldNormal = !BestHit.WorldNormal.IsNearlyZero() ? BestHit.WorldNormal : BroadHit.WorldNormal;
	OutPreciseHit.ImpactNormal = !BestHit.WorldNormal.IsNearlyZero() ? BestHit.WorldNormal : BroadHit.ImpactNormal;

	UE_LOG(
		"[SniperDebug] PhysicsAsset precise hit accepted: Actor=%s Bone=%s Body=%d Shape=%d Distance=%.2f",
		BroadHit.HitActor ? BroadHit.HitActor->GetName().c_str() : "None",
		BestHit.BoneName.ToString().c_str(),
		BestHit.BodyIndex,
		BestHit.ShapeIndex,
		OutPreciseHit.Distance);

	return true;
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
	UE_LOG(
		"[SniperDebug] Bullet hit: Actor=%s Component=%s RawBone=%s ResolvedBone=%s Region=%d Distance=%.2f Speed=%.2f BodyCenterDistance=%.3f HasBodyCenterDistance=%d",
		HitInfo.HitActor ? HitInfo.HitActor->GetName().c_str() : "None",
		Hit.HitComponent ? Hit.HitComponent->GetName().c_str() : "None",
		Hit.HitBoneName.ToString().c_str(),
		HitInfo.HitBoneName.ToString().c_str(),
		static_cast<int32>(HitInfo.HitRegion),
		HitInfo.TravelDistance,
		HitInfo.ImpactSpeed,
		HitInfo.HitBodyCenterDistance,
		HitInfo.bHasHitBodyCenterDistance ? 1 : 0);
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

	if (ShouldNotifyKillCamForHit(HitInfo))
	{
		ASniperKillCamDirector::NotifyBulletHit(HitInfo);
	}
	else
	{
		UE_LOG(
			"[SniperDebug] KillCam skipped by precision filter: Actor=%s Bone=%s Distance=%.3f Max=%.3f HasDistance=%d",
			HitInfo.HitActor ? HitInfo.HitActor->GetName().c_str() : "None",
			HitInfo.HitBoneName.ToString().c_str(),
			HitInfo.HitBodyCenterDistance,
			MaxKillCamBodyCenterDistance,
			HitInfo.bHasHitBodyCenterDistance ? 1 : 0);
	}
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
	HitInfo.HitBodyName = ResolvedHitBoneName.IsValid() && ResolvedHitBoneName != FName::None
		? ResolvedHitBoneName.ToString()
		: FString();
	HitInfo.HitRegionName = GetSniperHitRegionName(HitRegion);
	HitInfo.HitRegionDisplayName = GetSniperHitRegionDisplayName(HitRegion);
	HitInfo.HitScoreMultiplier = GetDefaultSniperHitScoreMultiplier(HitRegion);
	HitInfo.HitScoreValue = GetDefaultSniperHitScoreValue(HitRegion);
	FVector HitBodyCenter = FVector::ZeroVector;
	float HitBodyCenterDistance = 0.0f;
	if (ResolveHitBodyCenterMetrics(Hit, ResolvedHitBoneName, HitBodyCenter, HitBodyCenterDistance))
	{
		HitInfo.bHasHitBodyCenterDistance = true;
		HitInfo.HitBodyCenterLocation = HitBodyCenter;
		HitInfo.HitBodyCenterDistance = HitBodyCenterDistance;
	}
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

bool UBallisticBulletManagerComponent::ResolveHitBodyCenterMetrics(
	const FHitResult& Hit,
	const FName& HitBoneName,
	FVector& OutBodyCenter,
	float& OutDistance) const
{
	OutBodyCenter = FVector::ZeroVector;
	OutDistance = 0.0f;
	if (!Hit.bHit || !HitBoneName.IsValid() || HitBoneName == FName::None)
	{
		return false;
	}

	USkeletalMeshComponent* SkeletalMeshComponent = ResolveHitSkeletalMeshComponent(Hit);
	if (!SkeletalMeshComponent)
	{
		return false;
	}

	UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetEffectivePhysicsAsset();
	if (!PhysicsAsset)
	{
		return false;
	}

	const int32 BodyIndex = PhysicsAsset->FindBodySetupIndexByBoneName(HitBoneName);
	if (BodyIndex < 0)
	{
		return false;
	}

	FPhysicsAssetPreviewPoseCache PoseCache;
	if (!PoseCache.Initialize(SkeletalMeshComponent, PhysicsAsset))
	{
		return false;
	}

	FTransform BodyWorld;
	if (!PoseCache.ComputeBodyWorldTransform(BodyIndex, BodyWorld))
	{
		return false;
	}

	OutBodyCenter = BodyWorld.Location;
	OutDistance = FVector::Distance(Hit.WorldHitLocation, OutBodyCenter);
	return true;
}

bool UBallisticBulletManagerComponent::ShouldNotifyKillCamForHit(const FSniperHitInfo& HitInfo) const
{
	if (!bEnableKillCamBodyCenterDistanceFilter)
	{
		return true;
	}

	if (!HitInfo.HitActor)
	{
		return false;
	}

	const bool bCharacterLikeHit =
		Cast<ACombatCharacter>(HitInfo.HitActor) ||
		HitInfo.HitActor->GetComponentByClass<USniperDamageReceiverComponent>() != nullptr;
	if (!bCharacterLikeHit)
	{
		return true;
	}

	const FString NormalizedHitBoneName = NormalizeBoneNameForHitClassification(HitInfo.HitBoneName);
	const bool bKillCamEligibleBone =
		HasNormalizedBoneToken(NormalizedHitBoneName, "head") ||
		HasNormalizedBoneToken(NormalizedHitBoneName, "spine") ||
		HasNormalizedBoneToken(NormalizedHitBoneName, "pelvis");
	if (!bKillCamEligibleBone)
	{
		return false;
	}

	return HitInfo.bHasHitBodyCenterDistance &&
		HitInfo.HitBodyCenterDistance <= (std::max)(MaxKillCamBodyCenterDistance, 0.0f);
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
