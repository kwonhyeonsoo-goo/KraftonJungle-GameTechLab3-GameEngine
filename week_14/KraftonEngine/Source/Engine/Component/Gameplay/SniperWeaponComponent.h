#pragma once

#include "Component/ActorComponent.h"
#include "Component/Gameplay/SniperTypes.h"
#include "Object/Ptr/WeakObjectPtr.h"

#include "Source/Engine/Component/Gameplay/SniperWeaponComponent.generated.h"

class UBallisticBulletManagerComponent;

UCLASS()
class USniperWeaponComponent : public UActorComponent
{
public:
	GENERATED_BODY()
	USniperWeaponComponent();
	~USniperWeaponComponent() override = default;

	void BeginPlay() override;
	void EndPlay() override;

	UFUNCTION(Pure, Category="Sniper|Weapon")
	ESniperAmmoType GetCurrentAmmoType() const { return CurrentAmmoType; }
	UFUNCTION(Callable, Category="Sniper|Weapon")
	bool SetCurrentAmmoType(ESniperAmmoType InAmmoType);
	const FAmmoBallisticData* GetCurrentAmmoData() const;
	const FAmmoBallisticData* GetAmmoData(ESniperAmmoType InAmmoType) const;
	UFUNCTION(Pure, Category="Sniper|Weapon")
	bool CanFire() const;
	UFUNCTION(Callable, Category="Sniper|Weapon")
	bool RequestFire(const FVector& MuzzlePosition, const FVector& ShotDirection, bool bWasScopedShot, AActor* Shooter);
	UFUNCTION(Callable, Category="Sniper|Weapon")
	void NotifySniperHit(const FSniperHitInfo& HitInfo);
	UFUNCTION(Pure, Category="Sniper|Weapon")
	float GetFireCooldownRemaining() const { return FireCooldownRemaining; }
	UFUNCTION(Pure, Category="Sniper|Weapon")
	UBallisticBulletManagerComponent* GetBulletManagerComponent() const { return BulletManagerComponent.Get(); }

	FSniperHitEventSignature OnSniperHit;

protected:
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;

private:
	void InitializeDefaultAmmoData();
	void ResolveBulletManagerComponent();

	ESniperAmmoType CurrentAmmoType = ESniperAmmoType::Normal;
	float FireCooldownRemaining = 0.0f;
	TArray<FAmmoBallisticData> AmmoBallisticTable;
	TWeakObjectPtr<UBallisticBulletManagerComponent> BulletManagerComponent;
};
