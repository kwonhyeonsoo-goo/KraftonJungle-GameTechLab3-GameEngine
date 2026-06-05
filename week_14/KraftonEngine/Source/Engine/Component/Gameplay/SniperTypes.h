#pragma once

#include "Core/Delegate.h"
#include "Core/Types/CoreTypes.h"
#include "Math/Vector.h"
#include "Object/FName.h"
#include "Object/Reflection/ObjectMacros.h"
#include "Object/Reflection/UStruct.h"

#include "Source/Engine/Component/Gameplay/SniperTypes.generated.h"

class AActor;

UENUM()
enum class ESniperAmmoType : uint8
{
	Normal = 0,
	AntiMaterial = 1,

	COUNT
};

USTRUCT()
struct FSniperInputState
{
	GENERATED_BODY()

	float MouseDeltaX = 0.0f;
	float MouseDeltaY = 0.0f;
	bool bFirePressed = false;
	bool bScopeHeld = false;
	bool bHoldBreathHeld = false;
	bool bReloadPressed = false;
	bool bSwitchAmmoPressed = false;
};

USTRUCT()
struct FScopeState
{
	GENERATED_BODY()

	bool bIsScoped = false;
	float NormalFOV = 1.22173048f;
	float ScopedFOV = 0.26179939f;
	float CurrentFOV = 1.22173048f;
	float TargetFOV = 1.22173048f;
	float NormalSensitivity = 1.0f;
	float ScopedSensitivity = 0.25f;
	float CurrentSensitivity = 1.0f;
	float ScopeBlendSpeed = 12.0f;
};

USTRUCT()
struct FAimSwayState
{
	GENERATED_BODY()

	float Time = 0.0f;
	float BaseSwayAmount = 0.00034907f;
	float ScopedSwayAmount = 0.00139626f;
	float CurrentSwayPitch = 0.0f;
	float CurrentSwayYaw = 0.0f;
	float BreathMultiplier = 1.0f;
	float HoldBreathGauge = 3.0f;
	float MaxHoldBreathGauge = 3.0f;
	float HoldBreathRecoverSpeed = 0.5f;
	float HoldBreathConsumeSpeed = 1.0f;
};

USTRUCT()
struct FRecoilState
{
	GENERATED_BODY()

	float CurrentRecoilPitch = 0.0f;
	float CurrentRecoilYaw = 0.0f;
	float RecoilRecoverSpeed = 8.0f;
	float LastShotRecoilPitch = 0.0f;
	float LastShotRecoilYaw = 0.0f;
};

USTRUCT()
struct FAmmoBallisticData
{
	GENERATED_BODY()

	ESniperAmmoType AmmoType = ESniperAmmoType::Normal;
	float InitialSpeed = 900.0f;
	float GravityScale = 1.0f;
	float DragCoefficient = 0.0f;
	float Damage = 100.0f;
	float BulletRadius = 0.03f;
	float LifeTime = 5.0f;
	float FireInterval = 1.0f;
	float WindInfluenceScale = 1.0f;
	float RecoilPitch = 1.2f;
	float RecoilYawRandomRange = 0.25f;
	bool bCanDamageArmor = false;
};

USTRUCT()
struct FBallisticBullet
{
	GENERATED_BODY()

	FVector Position = FVector::ZeroVector;
	FVector PreviousPosition = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float Damage = 0.0f;
	float Radius = 0.0f;
	float LifeTime = 0.0f;
	float GravityScale = 1.0f;
	float DragCoefficient = 0.0f;
	float WindInfluenceScale = 1.0f;
	ESniperAmmoType AmmoType = ESniperAmmoType::Normal;
	AActor* Owner = nullptr;
	bool bIsAlive = false;
	bool bWasScopedShot = false;
	bool bCanDamageArmor = false;
};

USTRUCT()
struct FSniperHitInfo
{
	GENERATED_BODY()

	AActor* HitActor = nullptr;
	FVector HitLocation = FVector::ZeroVector;
	FVector HitNormal = FVector::ZeroVector;
	FVector ShotDirection = FVector::ZeroVector;
	float Damage = 0.0f;
	ESniperAmmoType AmmoType = ESniperAmmoType::Normal;
	bool bIsScopedShot = false;
	bool bIsHeadshot = false;
	bool bIsArmorPiercing = false;
	AActor* Shooter = nullptr;
	FName HitBoneName = FName::None;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FSniperHitEventSignature, const FSniperHitInfo&);
