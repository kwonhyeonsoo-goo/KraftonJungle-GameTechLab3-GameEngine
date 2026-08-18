#pragma once

#include "EngineAPI.h"
#include "Actor.h"
#include "Input/InputManager.h"
#include "Input/InputAction.h"
#include "Core/Paths.h"

class AController;
class UCameraComponent;
class UBillboardComponent;
class FEnhancedInputManager;
struct FInputMappingContext;

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

	// EnhancedInput 기반 입력 바인딩 (서브클래스에서 override)
	virtual void SetupPlayerInputComponent(FEnhancedInputManager* EnhancedInput, FInputMappingContext* InputContext);

	// APawn은 Controller에 의해 Possess됩니다.
	virtual void PossessedBy(AController* NewController);
	virtual void UnPossessed();

	UCameraComponent* GetCameraComponent() const { return Camera; }

	virtual void DuplicateSubObjects() override;
protected:
	virtual FString GetDefaultBillboardIconPath() const;

	UCameraComponent* Camera = nullptr;
	FVector MoveDirection = FVector::Zero();

private:
	AController* Controller = nullptr;

	FInputAction MoveAction{ "IA_Move", EInputActionValueType::Axis2D };
	FInputAction LookAction{ "IA_Look", EInputActionValueType::Axis2D };
};