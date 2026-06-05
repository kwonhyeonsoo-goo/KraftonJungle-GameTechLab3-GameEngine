#pragma once

#include "GameFramework/Pawn/Pawn.h"
#include "Component/Gameplay/SniperTypes.h"
#include "Object/Ptr/WeakObjectPtr.h"

#include "Source/Engine/GameFramework/Pawn/SniperPawn.generated.h"

class UBallisticBulletManagerComponent;
class UCameraComponent;
class USceneComponent;
class USniperWeaponComponent;

UCLASS()
class ASniperPawn : public APawn
{
public:
	GENERATED_BODY()
	ASniperPawn();
	~ASniperPawn() override = default;

	void BeginPlay() override;
	void PostDuplicate() override;
	void SetupInputComponent() override;
	void Tick(float DeltaTime) override;

	void InitDefaultComponents();

	UFUNCTION(Pure, Category="Sniper|Components")
	USceneComponent* GetSniperRoot() const { return SniperRoot.Get(); }
	UFUNCTION(Pure, Category="Sniper|Components")
	UCameraComponent* GetCamera() const { return Camera.Get(); }
	UFUNCTION(Pure, Category="Sniper|Components")
	USniperWeaponComponent* GetSniperWeaponComponent() const { return WeaponComponent.Get(); }
	UFUNCTION(Pure, Category="Sniper|Components")
	UBallisticBulletManagerComponent* GetBallisticBulletManagerComponent() const { return BulletManagerComponent.Get(); }

private:
	void CacheComponentReferences();
	void SyncSniperRuntimeState();
	void UpdateScopeState(float DeltaTime);
	void UpdateHoldBreathState(float DeltaTime);
	void UpdateAimSwayState(float DeltaTime);
	void UpdateRecoilState(float DeltaTime);
	void ApplySniperControlRotation();
	FRotator BuildEffectiveAimRotation() const;
	void HandleTurnInput(float Value);
	void HandleLookUpInput(float Value);
	void HandleFirePressed();
	void HandleScopePressed();
	void HandleScopeReleased();
	void HandleHoldBreathPressed();
	void HandleHoldBreathReleased();
	void HandleSwitchAmmoNormalPressed();
	void HandleSwitchAmmoAntiMaterialPressed();
	void ApplyFireRecoil();
	bool FireCurrentRound();

	TWeakObjectPtr<USceneComponent> SniperRoot;
	TWeakObjectPtr<UCameraComponent> Camera;
	TWeakObjectPtr<USniperWeaponComponent> WeaponComponent;
	TWeakObjectPtr<UBallisticBulletManagerComponent> BulletManagerComponent;

	FSniperInputState InputState;
	FScopeState ScopeState;
	FAimSwayState AimSwayState;
	FRecoilState RecoilState;

	float MouseSensitivity = 0.2f;
	float MinCameraPitch = -80.0f;
	float MaxCameraPitch = 60.0f;
	bool bInvertMouseY = false;
};
