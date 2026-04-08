#pragma once

#include "EngineAPI.h"
#include "Actor.h"
#include "Input/InputManager.h"
#include "Core/Paths.h"
class AController;
class UCameraComponent;
class UBillboardComponent;

class ENGINE_API APawn : public AActor
{
public:
	DECLARE_RTTI(APawn, AActor)
	DECLARE_DUPLICATE(APawn)

	~APawn() override;

	virtual void PostSpawnInitialize() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay() override;

	virtual void ProcessInput(int32 KeyCode, EInputEventType EventType);

	// APawn은 Controller에 의해 Possess됩니다.
	// Possess/Unpossess 시점에 발생하는 이벤트 함수입니다.
	virtual void PossessedBy(AController* NewController);
	virtual void UnPossessed();

	virtual void DuplicateSubObjects() override;
protected:
	virtual FString GetDefaultBillboardIconPath() const;
private:
	AController* Controller = nullptr;

	UCameraComponent* Camera = nullptr;

	FVector MoveDirection = FVector::Zero();
};