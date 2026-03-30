#include "EditorViewportController.h"
#include "Component/CameraComponent.h"
#include "Input/InputManager.h"
#include "Component/SceneComponent.h"

void FEditorViewportController::Initialize(
	UCameraComponent* InCameraComp, FInputManager* InInput)
{
	CameraComponent = InCameraComp;
	InputManager = InInput;
	// 등록하는 콜백 없음 — FEnhancedInputManager 완전 제거
}

void FEditorViewportController::Tick(float DeltaTime)
{
	if (!bActive) return;       // SetActive → Tick 순서가 보장되므로 항상 최신값
	ProcessCameraInput(DeltaTime);
}

void FEditorViewportController::ProcessCameraInput(float DeltaTime)
{
	if (!InputManager || !CameraComponent) return;

	const bool bRMB = InputManager->IsMouseButtonDown(FInputManager::MOUSE_RIGHT);
	const bool bMMB = InputManager->IsMouseButtonDown(FInputManager::MOUSE_MIDDLE);

	if (bRMB)
	{
		const float Speed = CameraComponent->GetSpeed();  // 또는 CameraComponent에서 가져오기
		if (InputManager->IsKeyDown('W')) CameraComponent->MoveForward(Speed * DeltaTime);
		if (InputManager->IsKeyDown('S')) CameraComponent->MoveForward(-Speed * DeltaTime);
		if (InputManager->IsKeyDown('D')) CameraComponent->MoveRight(Speed * DeltaTime);
		if (InputManager->IsKeyDown('A')) CameraComponent->MoveRight(-Speed * DeltaTime);
		if (InputManager->IsKeyDown('E')) CameraComponent->MoveUp(Speed * DeltaTime);
		if (InputManager->IsKeyDown('Q')) CameraComponent->MoveUp(-Speed * DeltaTime);

		const float Sens = CameraComponent->GetSensitivity();
		CameraComponent->Rotate(InputManager->GetMouseDeltaX() * Sens,
			-InputManager->GetMouseDeltaY() * Sens);
	}
	else if (bMMB)
	{
		CameraComponent->PanRight(-InputManager->GetMouseDeltaX() * DeltaTime);
		CameraComponent->PanUp(InputManager->GetMouseDeltaY() * DeltaTime);
	}

	// 휠은 FInputManager::Tick()이 매 프레임 0으로 리셋하므로 그냥 읽으면 됨
	const float Wheel = InputManager->GetMouseWheelDelta();
	if (Wheel != 0.f)
		CameraComponent->Zoom(-Wheel);
}

void FEditorViewportController::SetFocus(USceneComponent* InFocusTarget)
{
	CameraComponent->SetFocus(InFocusTarget);
}