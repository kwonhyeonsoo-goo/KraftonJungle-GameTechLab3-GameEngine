#include "Component/CameraComponent.h"
#include "Input/InputManager.h"
#include "Component/SceneComponent.h"
#include "OrthoViewportController.h"

void FOrthoViewportController::ProcessCameraInput(float DeltaTime)
{
	KeyBoardProcess(DeltaTime);
	MouseProcess(DeltaTime);
}


void FOrthoViewportController::MouseProcess(float DeltaTime)
{
	const bool bRMB = InputManager->IsMouseButtonDown(FInputManager::MOUSE_RIGHT);
	const bool bMMB = InputManager->IsMouseButtonDown(FInputManager::MOUSE_MIDDLE);

	if (bRMB)
	{
		CameraComponent->PanRight(-InputManager->GetMouseDeltaX() * DeltaTime);
		CameraComponent->PanUp(InputManager->GetMouseDeltaY() * DeltaTime);
	}
	// 휠은 FInputManager::Tick()이 매 프레임 0으로 리셋하므로 그냥 읽으면 됨
	const float Wheel = InputManager->GetMouseWheelDelta();
	if (Wheel != 0.f)
		CameraComponent->Zoom(-Wheel);


}