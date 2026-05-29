#pragma once

#include "GameFramework/AActor.h"
#include "Object/Ptr/WeakObjectPtr.h"

#include "Source/Engine/GameFramework/Actor/CapsuleActor.generated.h"
class UCapsuleComponent;

UCLASS()
class ACapsuleActor : public AActor
{
public:
	GENERATED_BODY()
	ACapsuleActor() = default;

	void InitDefaultComponents();
	void PostDuplicate() override;

	UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }

private:
	TWeakObjectPtr<UCapsuleComponent> CapsuleComponent = nullptr;
};
