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
	void EndPlay() override;
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

	bool IsScoped() const { return ScopeState.bIsScoped; }
	bool IsHoldBreathInputHeld() const { return InputState.bHoldBreathHeld; }
	bool IsHoldBreathActive() const { return InputState.bHoldBreathHeld && ScopeState.bIsScoped && AimSwayState.HoldBreathGauge > 0.0f; }
	float GetHoldBreathGauge() const { return AimSwayState.HoldBreathGauge; }
	float GetMaxHoldBreathGauge() const { return AimSwayState.MaxHoldBreathGauge; }
	float GetHoldBreathDuration() const
	{
		return AimSwayState.HoldBreathConsumeSpeed > 0.0f
			? AimSwayState.MaxHoldBreathGauge / AimSwayState.HoldBreathConsumeSpeed
			: 0.0f;
	}
	float GetHoldBreathGaugeRatio() const
	{
		if (AimSwayState.MaxHoldBreathGauge <= 0.0f)
		{
			return 0.0f;
		}

		const float Ratio = AimSwayState.HoldBreathGauge / AimSwayState.MaxHoldBreathGauge;
		return Ratio < 0.0f ? 0.0f : (Ratio > 1.0f ? 1.0f : Ratio);
	}

	UPROPERTY(Edit, Save, Category="Sniper|Input", DisplayName="Mouse Sensitivity", Min=0.0f, Max=10.0f, Speed=0.01f)
	float MouseSensitivity = 0.2f;
	UPROPERTY(Edit, Save, Category="Sniper|Input", DisplayName="Min Camera Pitch", Min=-89.0f, Max=89.0f, Speed=0.1f)
	float MinCameraPitch = -80.0f;
	UPROPERTY(Edit, Save, Category="Sniper|Input", DisplayName="Max Camera Pitch", Min=-89.0f, Max=89.0f, Speed=0.1f)
	float MaxCameraPitch = 60.0f;
	UPROPERTY(Edit, Save, Category="Sniper|Input", DisplayName="Invert Mouse Y")
	bool bInvertMouseY = false;

	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="Scoped FOV", Member=ScopeState.ScopedFOV, Type=Float, Min=0.05f, Max=3.14f, Speed=0.01f);
	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="Normal Sensitivity", Member=ScopeState.NormalSensitivity, Type=Float, Min=0.0f, Max=10.0f, Speed=0.01f);
	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="Scoped Sensitivity", Member=ScopeState.ScopedSensitivity, Type=Float, Min=0.0f, Max=10.0f, Speed=0.01f);
	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="FOV Blend Speed", Member=ScopeState.ScopeBlendSpeed, Type=Float, Min=0.0f, Max=60.0f, Speed=0.1f);

	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Radius", Min=0.01f, Max=1.0f, Speed=0.01f)
	float ScopeLensRadius = 0.42f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Feather", Min=0.001f, Max=0.5f, Speed=0.01f)
	float ScopeLensFeather = 0.08f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Outer Blur Radius", Min=0.0f, Max=32.0f, Speed=0.1f)
	float ScopeLensOuterBlurRadius = 4.0f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Edge Blur Radius", Min=0.0f, Max=16.0f, Speed=0.1f)
	float ScopeLensEdgeBlurRadius = 1.5f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Intensity", Min=0.0f, Max=1.0f, Speed=0.01f)
	float ScopeLensIntensity = 1.0f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Blend Time", Min=0.0f, Max=2.0f, Speed=0.01f)
	float ScopeLensBlendTime = 0.08f;

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
};
