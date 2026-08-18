#pragma once

#include "EngineAPI.h"
#include "Actor.h"
#include "Input/InputManager.h"

class APawn;
class UCameraComponent;
class FEnhancedInputManager;
struct FInputMappingContext;

class ENGINE_API AController : public AActor
{
public:
	DECLARE_RTTI(AController, AActor)
	DECLARE_DUPLICATE(AController)

	~AController() override;

public:
	void PostSpawnInitialize() override;

	void BeginPlay() override;
	void Tick(float DeltaTime) override;
	void EndPlay() override;

	void Possess(APawn* InPawn);

	// EnhancedInput 시스템 연결
	void SetupInput(FInputManager* InInputManager, FEnhancedInputManager* InEnhancedInput);

	virtual void DuplicateSubObjects() override;

	UCameraComponent* GetCamera() const { return Camera; }
	void SetCamera(UCameraComponent* InCamera) { Camera = InCamera; }

	APawn* GetPawn() const { return Pawn; }

private:
	void SetupPawnInput();
	void ClearPawnInput();

	APawn* Pawn = nullptr;
	UCameraComponent* Camera = nullptr;

	FInputManager* InputManager = nullptr;
	FEnhancedInputManager* EnhancedInput = nullptr;
	FInputMappingContext* PawnInputContext = nullptr;
};
