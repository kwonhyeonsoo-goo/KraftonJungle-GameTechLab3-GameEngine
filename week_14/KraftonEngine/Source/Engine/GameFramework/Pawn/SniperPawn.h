#pragma once

#include "GameFramework/Pawn/Pawn.h"
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

	TWeakObjectPtr<USceneComponent> SniperRoot;
	TWeakObjectPtr<UCameraComponent> Camera;
	TWeakObjectPtr<USniperWeaponComponent> WeaponComponent;
	TWeakObjectPtr<UBallisticBulletManagerComponent> BulletManagerComponent;
};
