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
	UFUNCTION(Pure, Category="Sniper|State")
	bool IsScoped() const { return ScopeState.bIsScoped; }
	UFUNCTION(Pure, Category="Sniper|State")
	bool IsReloading() const;
	UFUNCTION(Pure, Category="Sniper|State")
	float GetReloadRemaining() const;
	UFUNCTION(Pure, Category="Sniper|State")
	float GetReloadProgress() const;
	UFUNCTION(Callable, Category="Sniper|State")
	void ForceScopeReleased();
	UFUNCTION(Pure, Category="Sniper|State")
	float GetScopeBlendAlpha() const;
	UFUNCTION(Pure, Category="Sniper|State")
	float GetCurrentScopeFOV() const { return ScopeState.CurrentFOV; }
	UFUNCTION(Pure, Category="Sniper|State")
	float GetCurrentScopeZoomMagnification() const { return ScopeState.CurrentZoomMagnification; }
	UFUNCTION(Pure, Category="Sniper|State")
	float GetMinScopeZoomMagnification() const { return ScopeState.MinZoomMagnification; }
	UFUNCTION(Pure, Category="Sniper|State")
	float GetMaxScopeZoomMagnification() const { return ScopeState.MaxZoomMagnification; }
	UFUNCTION(Pure, Category="Sniper|State")
	float GetCurrentScopeSensitivity() const { return ScopeState.CurrentSensitivity; }
	UFUNCTION(Pure, Category="Sniper|State")
	bool IsHoldBreathActive() const;
	UFUNCTION(Pure, Category="Sniper|State")
	float GetHoldBreathGauge() const { return AimSwayState.HoldBreathGauge; }
	UFUNCTION(Pure, Category="Sniper|State")
	float GetMaxHoldBreathGauge() const { return AimSwayState.MaxHoldBreathGauge; }

	UFUNCTION(Pure, Category="Sniper|State")
	bool IsHoldBreathInputHeld() const { return InputState.bHoldBreathHeld; }
	UFUNCTION(Pure, Category="Sniper|State")
	bool IsHoldBreathRecovering() const { return AimSwayState.bForcedRecovery; }
	UFUNCTION(Pure, Category="Sniper|State")
	bool IsHoldBreathReleaseRequired() const { return AimSwayState.bRequireHoldBreathRelease; }
	UFUNCTION(Pure, Category="Sniper|State")
	bool IsHoldBreathOnCooldown() const { return AimSwayState.HoldBreathCooldownRemaining > 0.0f; }
	UFUNCTION(Pure, Category="Sniper|State")
	float GetHoldBreathCooldownRemaining() const { return AimSwayState.HoldBreathCooldownRemaining; }
	UFUNCTION(Pure, Category="Sniper|State")
	float GetHoldBreathDuration() const
	{
		return AimSwayState.HoldBreathConsumeSpeed > 0.0f
			? AimSwayState.MaxHoldBreathGauge / AimSwayState.HoldBreathConsumeSpeed
			: 0.0f;
	}
	UFUNCTION(Pure, Category="Sniper|State")
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
	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="Min Zoom Magnification", Member=ScopeState.MinZoomMagnification, Type=Float, Min=1.0f, Max=64.0f, Speed=0.1f);
	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="Max Zoom Magnification", Member=ScopeState.MaxZoomMagnification, Type=Float, Min=1.0f, Max=64.0f, Speed=0.1f);
	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="Default Zoom Magnification", Member=ScopeState.DefaultZoomMagnification, Type=Float, Min=1.0f, Max=64.0f, Speed=0.1f);
	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="Zoom Step", Member=ScopeState.ZoomStep, Type=Float, Min=0.1f, Max=16.0f, Speed=0.1f);
	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="Normal Sensitivity", Member=ScopeState.NormalSensitivity, Type=Float, Min=0.0f, Max=10.0f, Speed=0.01f);
	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="Min Zoom Scoped Sensitivity", Member=ScopeState.ScopedSensitivity, Type=Float, Min=0.0f, Max=10.0f, Speed=0.01f);
	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="Max Zoom Scoped Sensitivity", Member=ScopeState.MaxZoomScopedSensitivity, Type=Float, Min=0.0f, Max=10.0f, Speed=0.01f);
	UPROPERTY(Edit, Save, Category="Sniper|Scope", DisplayName="FOV Blend Speed", Member=ScopeState.ScopeBlendSpeed, Type=Float, Min=0.0f, Max=60.0f, Speed=0.1f);

	UPROPERTY(Edit, Save, Category="Sniper|Aim Sway", DisplayName="Base Sway Amount", Member=AimSwayState.BaseSwayAmount, Type=Float, Min=0.0f, Max=0.05f, Speed=0.0001f);
	UPROPERTY(Edit, Save, Category="Sniper|Aim Sway", DisplayName="Scoped Sway Amount", Member=AimSwayState.ScopedSwayAmount, Type=Float, Min=0.0f, Max=0.05f, Speed=0.0001f);
	UPROPERTY(Edit, Save, Category="Sniper|Aim Sway", DisplayName="Hold Breath Sway Multiplier", Min=0.0f, Max=1.0f, Speed=0.01f)
	float HoldBreathSwayMultiplier = 0.04f;
	UPROPERTY(Edit, Save, Category="Sniper|Aim Sway", DisplayName="Exhausted Sway Multiplier", Min=1.0f, Max=10.0f, Speed=0.1f)
	float ExhaustedSwayMultiplier = 10.0f;
	UPROPERTY(Edit, Save, Category="Sniper|Aim Sway", DisplayName="Hold Breath Sway Blend Speed", Min=0.0f, Max=60.0f, Speed=0.1f)
	float HoldBreathSwayBlendSpeed = 4.0f;
	UPROPERTY(Edit, Save, Category="Sniper|Aim Sway", DisplayName="Hold Breath Reentry Delay", Min=0.0f, Max=10.0f, Speed=0.1f)
	float HoldBreathReentryDelay = 2.0f;
	UPROPERTY(Edit, Save, Category="Sniper|Aim Sway", DisplayName="Sway Pitch Frequency", Min=0.0f, Max=20.0f, Speed=0.01f)
	float SwayPitchFrequency = 1.85f;
	UPROPERTY(Edit, Save, Category="Sniper|Aim Sway", DisplayName="Sway Yaw Frequency", Min=0.0f, Max=20.0f, Speed=0.01f)
	float SwayYawFrequency = 1.43f;
	UPROPERTY(Edit, Save, Category="Sniper|Aim Sway", DisplayName="Max Hold Breath Gauge", Member=AimSwayState.MaxHoldBreathGauge, Type=Float, Min=0.0f, Max=30.0f, Speed=0.1f);
	UPROPERTY(Edit, Save, Category="Sniper|Aim Sway", DisplayName="Hold Breath Recover Speed", Member=AimSwayState.HoldBreathRecoverSpeed, Type=Float, Min=0.0f, Max=30.0f, Speed=0.1f);
	UPROPERTY(Edit, Save, Category="Sniper|Aim Sway", DisplayName="Hold Breath Consume Speed", Member=AimSwayState.HoldBreathConsumeSpeed, Type=Float, Min=0.0f, Max=30.0f, Speed=0.1f);

	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Radius", Min=0.01f, Max=1.0f, Speed=0.01f)
	float ScopeLensRadius = 0.688889f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Center X", Min=0.0f, Max=1.0f, Speed=0.001f)
	float ScopeLensCenterX = 0.5f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Center Y", Min=0.0f, Max=1.0f, Speed=0.001f)
	float ScopeLensCenterY = 0.5f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Feather", Min=0.001f, Max=0.5f, Speed=0.01f)
	float ScopeLensFeather = 0.08f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Outer Blur Radius", Min=0.0f, Max=32.0f, Speed=0.1f)
	float ScopeLensOuterBlurRadius = 4.0f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Edge Blur Radius", Min=0.0f, Max=16.0f, Speed=0.1f)
	float ScopeLensEdgeBlurRadius = 1.5f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Intensity", Min=0.0f, Max=1.0f, Speed=0.01f)
	float ScopeLensIntensity = 1.0f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Center Offset X", Min=-1.0f, Max=1.0f, Speed=0.001f)
	float ScopeLensCenterOffsetX = 0.0f;
	UPROPERTY(Edit, Save, Category="Sniper|Scope Lens", DisplayName="Center Offset Y", Min=-1.0f, Max=1.0f, Speed=0.001f)
	float ScopeLensCenterOffsetY = -0.105556f;
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
	bool CanEnterScope() const;
	float ClampScopeZoomMagnification(float Magnification) const;
	float ComputeScopedFOVForMagnification(float Magnification) const;
	float ComputeScopedSensitivityForMagnification(float Magnification) const;
	void AdjustScopeZoomStep(int32 StepDelta);
	void HandleTurnInput(float Value);
	void HandleLookUpInput(float Value);
	void HandleScopeZoomAxis(float Value);
	void HandleFirePressed();
	void HandleScopePressed();
	void HandleScopeReleased();
	void HandleHoldBreathPressed();
	void HandleHoldBreathReleased();
	void HandleSwitchAmmoNormalPressed();
	void HandleSwitchAmmoAntiMaterialPressed();
	void HandleReloadPressed();
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
