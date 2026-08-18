#pragma once

#include "Input/InputAction.h"

class UCameraComponent;
class FInputManager;
class FEnhancedInputManager;
struct FInputMappingContext;

class FEditorViewportController
{
public:
	~FEditorViewportController();

	void Initialize(UCameraComponent* InCameraComp, FInputManager* InInput, FEnhancedInputManager* InEnhancedInput);
	void Cleanup();
	void Tick(float DeltaTime);

private:
	void SetupInputBindings();

	UCameraComponent* CameraComponent = nullptr;
	FInputManager* InputManager = nullptr;
	FEnhancedInputManager* EnhancedInput = nullptr;
	FInputMappingContext* CameraContext = nullptr;

	FInputAction MoveForwardAction{ "MoveForward", EInputActionValueType::Float };
	FInputAction MoveRightAction{ "MoveRight",   EInputActionValueType::Float };
	FInputAction MoveUpAction{ "MoveUp",      EInputActionValueType::Float };
	FInputAction LookXAction{ "LookX",       EInputActionValueType::Float };
	FInputAction LookYAction{ "LookY",       EInputActionValueType::Float };
	FInputAction LookWheelAction{ "LookWheel", EInputActionValueType::Float };

	float CurrentDeltaTime = 0.0f; // 콜백에서 DeltaTime 쓰기 위해 보관
};
