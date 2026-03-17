#include "EditorViewportController.h"
#include "Component/CameraComponent.h"
#include "Input/InputManager.h"


void CEditorViewportController::Initialize(UCameraComponent* InCameraComp, CInputManager* InInput)
{
	CameraComponent = InCameraComp;
	InputManager = InInput;
}

void CEditorViewportController::Tick(float DeltaTime)
{
	ProcessCameraInput(DeltaTime);
}

void CEditorViewportController::ProcessCameraInput(float DeltaTime)
{
	if (InputManager->IsKeyDown('W')) CameraComponent->MoveForward(DeltaTime);
	if (InputManager->IsKeyDown('S')) CameraComponent->MoveForward(-DeltaTime);
	if (InputManager->IsKeyDown('D')) CameraComponent->MoveRight(DeltaTime);
	if (InputManager->IsKeyDown('A')) CameraComponent->MoveRight(-DeltaTime);
	if (InputManager->IsKeyDown('E')) CameraComponent->MoveUp(DeltaTime);
	if (InputManager->IsKeyDown('Q')) CameraComponent->MoveUp(-DeltaTime);

	if (InputManager->IsMouseButtonDown(CInputManager::MOUSE_RIGHT))
	{
		float DeltaX = InputManager->GetMouseDeltaX();
		float DeltaY = InputManager->GetMouseDeltaY();
		CameraComponent->Rotate(DeltaX * 0.2f, -DeltaY * 0.2f);
	}
}
