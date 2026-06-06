#include "Component/Gameplay/SniperDamageReceiverComponent.h"

#include "Math/MathUtils.h"

namespace
{
	constexpr float SniperRicochetAlignmentThreshold = 0.45f;
	constexpr float SniperArmorPenetrationSpeedScale = 350.0f;
	constexpr float SniperBlockedDamageMultiplier = 0.15f;
	constexpr float SniperPenetratedDamageMultiplier = 0.75f;
}

USniperDamageReceiverComponent::USniperDamageReceiverComponent()
{
	bTickEnable = false;
}

void USniperDamageReceiverComponent::BeginPlay()
{
	UActorComponent::BeginPlay();
	ResetHealth();
}

void USniperDamageReceiverComponent::EndPlay()
{
	UActorComponent::EndPlay();
}

bool USniperDamageReceiverComponent::CanReceiveSniperHit() const
{
	return !bIsDead && CurrentHP > 0.0f;
}

FSniperHitInfo USniperDamageReceiverComponent::ResolveSniperHit(const FSniperHitInfo& HitInfo) const
{
	return BuildResolvedHitInfo(HitInfo);
}

void USniperDamageReceiverComponent::ResetHealth()
{
	if (MaxHP < 1.0f)
	{
		MaxHP = 1.0f;
	}

	CurrentHP = FMath::Clamp(CurrentHP, 0.0f, MaxHP);
	if (CurrentHP <= 0.0f || CurrentHP > MaxHP)
	{
		CurrentHP = MaxHP;
	}

	bIsDead = false;
}

bool USniperDamageReceiverComponent::ApplySniperHit(const FSniperHitInfo& HitInfo)
{
	return ApplyResolvedSniperHit(BuildResolvedHitInfo(HitInfo));
}

bool USniperDamageReceiverComponent::ApplyResolvedSniperHit(const FSniperHitInfo& HitInfo)
{
	if (!CanReceiveSniperHit())
	{
		return false;
	}

	const float AppliedDamage = HitInfo.Damage < 0.0f ? 0.0f : HitInfo.Damage;
	CurrentHP = FMath::Clamp(CurrentHP - AppliedDamage, 0.0f, MaxHP);

	FSniperHitInfo ResolvedHitInfo = HitInfo;
	ResolvedHitInfo.Damage = AppliedDamage;
	OnSniperDamaged.Broadcast(ResolvedHitInfo);

	if (CurrentHP <= 0.0f)
	{
		bIsDead = true;
		OnSniperKilled.Broadcast(ResolvedHitInfo);
	}

	return true;
}

FSniperHitInfo USniperDamageReceiverComponent::BuildResolvedHitInfo(const FSniperHitInfo& HitInfo) const
{
	FSniperHitInfo ResolvedHitInfo = HitInfo;

	const FVector SurfaceNormal = HitInfo.HitNormal.IsNearlyZero()
		? FVector::UpVector
		: HitInfo.HitNormal.Normalized();
	const FVector IncomingDirection = HitInfo.ShotDirection.IsNearlyZero()
		? FVector::ForwardVector
		: HitInfo.ShotDirection.Normalized();
	const FVector ReverseIncomingDirection = IncomingDirection * -1.0f;
	const float SurfaceAlignment = FMath::Clamp(ReverseIncomingDirection.Dot(SurfaceNormal), 0.0f, 1.0f);

	ResolvedHitInfo.HitOutcome = ESniperHitOutcome::Normal;
	ResolvedHitInfo.bShouldRagdoll = HitInfo.bShouldRagdoll && bCanRagdoll;

	if (bHasArmor)
	{
		const bool bRicochet = bAllowRicochet && SurfaceAlignment <= SniperRicochetAlignmentThreshold;
		const float PenetrationThreshold = ArmorStrength * SniperArmorPenetrationSpeedScale;
		const bool bCanPenetrateArmor = HitInfo.bIsArmorPiercing && HitInfo.ImpactSpeed >= PenetrationThreshold;

		if (bRicochet)
		{
			ResolvedHitInfo.HitOutcome = ESniperHitOutcome::Ricochet;
			ResolvedHitInfo.Damage = 0.0f;
			ResolvedHitInfo.bShouldRagdoll = false;
			ResolvedHitInfo.RagdollImpulseStrength = 0.0f;
		}
		else if (bCanPenetrateArmor)
		{
			ResolvedHitInfo.HitOutcome = ESniperHitOutcome::Penetrated;
			ResolvedHitInfo.Damage = HitInfo.Damage * SniperPenetratedDamageMultiplier;
		}
		else
		{
			ResolvedHitInfo.HitOutcome = ESniperHitOutcome::Blocked;
			ResolvedHitInfo.Damage = HitInfo.Damage * SniperBlockedDamageMultiplier;
			ResolvedHitInfo.bShouldRagdoll = false;
			ResolvedHitInfo.RagdollImpulseStrength *= 0.35f;
		}
	}

	return ResolvedHitInfo;
}
