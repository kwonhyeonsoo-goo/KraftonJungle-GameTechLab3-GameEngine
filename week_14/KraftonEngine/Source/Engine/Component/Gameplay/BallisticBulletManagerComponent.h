#pragma once

#include "Component/ActorComponent.h"
#include "Component/Gameplay/SniperTypes.h"
#include "Object/Ptr/WeakObjectPtr.h"

#include "Source/Engine/Component/Gameplay/BallisticBulletManagerComponent.generated.h"

class USniperWeaponComponent;

UCLASS()
class UBallisticBulletManagerComponent : public UActorComponent
{
public:
	GENERATED_BODY()
	UBallisticBulletManagerComponent();
	~UBallisticBulletManagerComponent() override = default;

	void BeginPlay() override;
	void EndPlay() override;

	UFUNCTION(Callable, Category="Sniper|Bullet")
	bool SpawnBullet(const FBallisticBullet& Bullet);
	UFUNCTION(Callable, Category="Sniper|Bullet")
	void ResetBullets();
	UFUNCTION(Pure, Category="Sniper|Bullet")
	int32 GetAliveBulletCount() const { return static_cast<int32>(ActiveBullets.size()); }
	UFUNCTION(Pure, Category="Sniper|Bullet")
	const TArray<FBallisticBullet>& GetActiveBullets() const { return ActiveBullets; }
	UFUNCTION(Pure, Category="Sniper|Bullet")
	USniperWeaponComponent* GetWeaponComponent() const { return WeaponComponent.Get(); }

protected:
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;

private:
	void CompactDeadBullets();
	void ResolveWeaponComponent();

	TWeakObjectPtr<USniperWeaponComponent> WeaponComponent;
	TArray<FBallisticBullet> ActiveBullets;
};
