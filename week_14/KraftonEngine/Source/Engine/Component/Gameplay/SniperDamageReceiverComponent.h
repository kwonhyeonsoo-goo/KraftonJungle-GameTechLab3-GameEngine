#pragma once

#include "Component/ActorComponent.h"
#include "Component/Gameplay/SniperTypes.h"

#include "Source/Engine/Component/Gameplay/SniperDamageReceiverComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FSniperDamageReceiverEventSignature, const FSniperHitInfo&);

UCLASS()
class USniperDamageReceiverComponent : public UActorComponent
{
public:
	GENERATED_BODY()

	USniperDamageReceiverComponent();
	~USniperDamageReceiverComponent() override = default;

	void BeginPlay() override;
	void EndPlay() override;

	UFUNCTION(Pure, Category="Sniper|Damage")
	float GetMaxHP() const { return MaxHP; }
	UFUNCTION(Pure, Category="Sniper|Damage")
	float GetCurrentHP() const { return CurrentHP; }
	UFUNCTION(Pure, Category="Sniper|Damage")
	bool IsFriendly() const { return bIsFriendly; }
	UFUNCTION(Pure, Category="Sniper|Damage")
	bool HasArmor() const { return bHasArmor; }
	UFUNCTION(Pure, Category="Sniper|Damage")
	float GetArmorStrength() const { return ArmorStrength; }
	UFUNCTION(Pure, Category="Sniper|Damage")
	bool AllowsRicochet() const { return bAllowRicochet; }
	UFUNCTION(Pure, Category="Sniper|Damage")
	bool CanRagdoll() const { return bCanRagdoll; }
	UFUNCTION(Pure, Category="Sniper|Damage")
	bool IsDead() const { return bIsDead; }
	UFUNCTION(Pure, Category="Sniper|Damage")
	bool CanReceiveSniperHit() const;
	UFUNCTION(Pure, Category="Sniper|Damage")
	FSniperHitInfo ResolveSniperHit(const FSniperHitInfo& HitInfo) const;
	UFUNCTION(Callable, Category="Sniper|Damage")
	void ResetHealth();
	UFUNCTION(Callable, Category="Sniper|Damage")
	bool ApplySniperHit(const FSniperHitInfo& HitInfo);
	UFUNCTION(Callable, Category="Sniper|Damage")
	bool ApplyResolvedSniperHit(const FSniperHitInfo& HitInfo);

	FSniperDamageReceiverEventSignature OnSniperDamaged;
	FSniperDamageReceiverEventSignature OnSniperKilled;

private:
	FSniperHitInfo BuildResolvedHitInfo(const FSniperHitInfo& HitInfo) const;

private:
	UPROPERTY(Edit, Save, Category="Sniper|Damage", DisplayName="Max HP", Min=1.0f, Max=10000.0f, Speed=1.0f)
	float MaxHP = 100.0f;

	UPROPERTY(Edit, Save, Category="Sniper|Damage", DisplayName="Current HP", Min=0.0f, Max=10000.0f, Speed=1.0f)
	float CurrentHP = 100.0f;

	UPROPERTY(Edit, Save, Category="Sniper|Damage", DisplayName="Friendly Target")
	bool bIsFriendly = false;

	UPROPERTY(Edit, Save, Category="Sniper|Damage", DisplayName="Has Armor")
	bool bHasArmor = false;

	UPROPERTY(Edit, Save, Category="Sniper|Damage", DisplayName="Armor Strength", Min=0.1f, Max=20.0f, Speed=0.1f)
	float ArmorStrength = 2.5f;

	UPROPERTY(Edit, Save, Category="Sniper|Damage", DisplayName="Allow Ricochet")
	bool bAllowRicochet = true;

	UPROPERTY(Edit, Save, Category="Sniper|Damage", DisplayName="Can Ragdoll")
	bool bCanRagdoll = true;

	bool bIsDead = false;
};
