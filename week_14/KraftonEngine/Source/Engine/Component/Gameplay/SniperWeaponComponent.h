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
	UFUNCTION(Pure, Category="Sniper|Weapon")
	bool IsZeroingEnabled() const { return bEnableZeroing; }
	UFUNCTION(Callable, Category="Sniper|Weapon")
	void SetZeroingEnabled(bool bInEnableZeroing) { bEnableZeroing = bInEnableZeroing; }
	UFUNCTION(Pure, Category="Sniper|Weapon")
	float GetZeroRangeMeters() const { return ZeroRangeMeters; }
	UFUNCTION(Callable, Category="Sniper|Weapon")
	void SetZeroRangeMeters(float InZeroRangeMeters);
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
	FVector BuildZeroedShotDirection(const FVector& ShotDirection, const FAmmoBallisticData& AmmoData) const;
	void InitializeDefaultAmmoData();
	void ResolveBulletManagerComponent();

	UPROPERTY(Edit, Save, Category="Sniper|Weapon")
	bool bEnableZeroing = true;
	UPROPERTY(Edit, Save, Category="Sniper|Weapon")
	float ZeroRangeMeters = 200.0f;
	ESniperAmmoType CurrentAmmoType = ESniperAmmoType::Normal;
	float FireCooldownRemaining = 0.0f;
	TArray<FAmmoBallisticData> AmmoBallisticTable;
	TWeakObjectPtr<UBallisticBulletManagerComponent> BulletManagerComponent;
};
