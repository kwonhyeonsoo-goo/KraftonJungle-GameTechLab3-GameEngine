#include "Component/Gameplay/SniperWeaponComponent.h"

#include "Component/Gameplay/BallisticBulletManagerComponent.h"
#include "GameFramework/AActor.h"

USniperWeaponComponent::USniperWeaponComponent()
{
	bTickEnable = true;
	InitializeDefaultAmmoData();
}

void USniperWeaponComponent::BeginPlay()
{
	UActorComponent::BeginPlay();
	ResolveBulletManagerComponent();
}

void USniperWeaponComponent::EndPlay()
{
	FireCooldownRemaining = 0.0f;
	UActorComponent::EndPlay();
}

bool USniperWeaponComponent::SetCurrentAmmoType(ESniperAmmoType InAmmoType)
{
	if (!GetAmmoData(InAmmoType))
	{
		return false;
	}

	CurrentAmmoType = InAmmoType;
	return true;
}

const FAmmoBallisticData* USniperWeaponComponent::GetCurrentAmmoData() const
{
	return GetAmmoData(CurrentAmmoType);
}

const FAmmoBallisticData* USniperWeaponComponent::GetAmmoData(ESniperAmmoType InAmmoType) const
{
	for (const FAmmoBallisticData& Entry : AmmoBallisticTable)
	{
		if (Entry.AmmoType == InAmmoType)
		{
			return &Entry;
		}
	}

	return nullptr;
}

bool USniperWeaponComponent::CanFire() const
{
	return FireCooldownRemaining <= 0.0f && GetCurrentAmmoData() != nullptr;
}

bool USniperWeaponComponent::RequestFire(
	const FVector& MuzzlePosition,
	const FVector& ShotDirection,
	bool bWasScopedShot,
	AActor* Shooter)
{
	const FAmmoBallisticData* AmmoData = GetCurrentAmmoData();
	UBallisticBulletManagerComponent* BulletManager = BulletManagerComponent.Get();
	if (!AmmoData || !BulletManager)
	{
		return false;
	}

	if (FireCooldownRemaining > 0.0f || ShotDirection.IsNearlyZero())
	{
		return false;
	}

	FBallisticBullet Bullet;
	Bullet.Position = MuzzlePosition;
	Bullet.PreviousPosition = MuzzlePosition;
	Bullet.Velocity = ShotDirection.Normalized() * AmmoData->InitialSpeed;
	Bullet.Damage = AmmoData->Damage;
	Bullet.Radius = AmmoData->BulletRadius;
	Bullet.LifeTime = AmmoData->LifeTime;
	Bullet.GravityScale = AmmoData->GravityScale;
	Bullet.DragCoefficient = AmmoData->DragCoefficient;
	Bullet.WindInfluenceScale = AmmoData->WindInfluenceScale;
	Bullet.AmmoType = AmmoData->AmmoType;
	Bullet.Owner = Shooter;
	Bullet.bIsAlive = true;
	Bullet.bWasScopedShot = bWasScopedShot;
	Bullet.bCanDamageArmor = AmmoData->bCanDamageArmor;

	if (!BulletManager->SpawnBullet(Bullet))
	{
		return false;
	}

	FireCooldownRemaining = AmmoData->FireInterval;
	return true;
}

void USniperWeaponComponent::NotifySniperHit(const FSniperHitInfo& HitInfo)
{
	OnSniperHit.Broadcast(HitInfo);
}

void USniperWeaponComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction& ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
	(void)TickType;
	(void)ThisTickFunction;

	if (FireCooldownRemaining > 0.0f)
	{
		FireCooldownRemaining -= DeltaTime;
		if (FireCooldownRemaining < 0.0f)
		{
			FireCooldownRemaining = 0.0f;
		}
	}
}

void USniperWeaponComponent::InitializeDefaultAmmoData()
{
	AmmoBallisticTable.clear();

	FAmmoBallisticData NormalAmmo;
	NormalAmmo.AmmoType = ESniperAmmoType::Normal;
	NormalAmmo.InitialSpeed = 900.0f;
	NormalAmmo.GravityScale = 1.0f;
	NormalAmmo.DragCoefficient = 0.015f;
	NormalAmmo.Damage = 100.0f;
	NormalAmmo.BulletRadius = 0.03f;
	NormalAmmo.LifeTime = 5.0f;
	NormalAmmo.FireInterval = 1.0f;
	NormalAmmo.WindInfluenceScale = 1.0f;
	NormalAmmo.RecoilPitch = 1.2f;
	NormalAmmo.RecoilYawRandomRange = 0.25f;
	NormalAmmo.bCanDamageArmor = false;
	AmmoBallisticTable.push_back(NormalAmmo);

	FAmmoBallisticData AntiMaterialAmmo;
	AntiMaterialAmmo.AmmoType = ESniperAmmoType::AntiMaterial;
	AntiMaterialAmmo.InitialSpeed = 1200.0f;
	AntiMaterialAmmo.GravityScale = 0.9f;
	AntiMaterialAmmo.DragCoefficient = 0.0075f;
	AntiMaterialAmmo.Damage = 300.0f;
	AntiMaterialAmmo.BulletRadius = 0.05f;
	AntiMaterialAmmo.LifeTime = 5.0f;
	AntiMaterialAmmo.FireInterval = 1.5f;
	AntiMaterialAmmo.WindInfluenceScale = 0.7f;
	AntiMaterialAmmo.RecoilPitch = 2.5f;
	AntiMaterialAmmo.RecoilYawRandomRange = 0.5f;
	AntiMaterialAmmo.bCanDamageArmor = true;
	AmmoBallisticTable.push_back(AntiMaterialAmmo);
}

void USniperWeaponComponent::ResolveBulletManagerComponent()
{
	if (UBallisticBulletManagerComponent* Existing = BulletManagerComponent.Get())
	{
		if (Existing->GetOwner() == GetOwner())
		{
			return;
		}
	}

	BulletManagerComponent = GetOwner() ? GetOwner()->GetComponentByClass<UBallisticBulletManagerComponent>() : nullptr;
}
