#include "Component/Gameplay/SniperDamageReceiverComponent.h"

#include "Math/MathUtils.h"

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
	if (!CanReceiveSniperHit())
	{
		return false;
	}

	const float AppliedDamage = HitInfo.Damage < 0.0f ? 0.0f : HitInfo.Damage;
	CurrentHP = FMath::Clamp(CurrentHP - AppliedDamage, 0.0f, MaxHP);

	FSniperHitInfo ResolvedHitInfo = BuildResolvedHitInfo(HitInfo);
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
	ResolvedHitInfo.bShouldRagdoll = HitInfo.bShouldRagdoll && bCanRagdoll;
	return ResolvedHitInfo;
}
