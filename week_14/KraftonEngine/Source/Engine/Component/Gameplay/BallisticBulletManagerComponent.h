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
	UFUNCTION(Pure, Category="Sniper|Wind")
	bool IsWindEnabled() const { return bEnableWind; }
	UFUNCTION(Callable, Category="Sniper|Wind")
	void SetWindEnabled(bool bInEnableWind) { bEnableWind = bInEnableWind; }
	UFUNCTION(Pure, Category="Sniper|Wind")
	FVector GetWindAcceleration() const { return WindAcceleration; }
	UFUNCTION(Callable, Category="Sniper|Wind")
	void SetWindAcceleration(const FVector& InWindAcceleration) { WindAcceleration = InWindAcceleration; }

protected:
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;

private:
	void UpdateBullets(float DeltaTime);
	void UpdateSingleBullet(FBallisticBullet& Bullet, const FVector& WorldGravity, const FVector& AppliedWindAcceleration, float DeltaTime, class UWorld* World);
	void DrawWindDebug(class UWorld* World) const;
	bool QueryBulletHit(const FBallisticBullet& Bullet, class UWorld* World, struct FHitResult& OutHit) const;
	void HandleBulletHit(FBallisticBullet& Bullet, const struct FHitResult& Hit, class UWorld* World);
	FSniperHitInfo BuildSniperHitInfo(const FBallisticBullet& Bullet, const struct FHitResult& Hit) const;
	void CompactDeadBullets();
	void ResolveWeaponComponent();

	UPROPERTY(Edit, Save, Category="Sniper|Wind")
	bool bEnableWind = true;
	UPROPERTY(Edit, Save, Category="Sniper|Wind")
	FVector WindAcceleration = FVector(0.0f, 1.5f, 0.0f);

	TWeakObjectPtr<USniperWeaponComponent> WeaponComponent;
	TArray<FBallisticBullet> ActiveBullets;
};
