#pragma once

class UCameraComponent;
class CInputManager;

class CEditorViewportController
{
public:
	void Initialize(UCameraComponent* InCameraComp, CInputManager* InInput);
	void Tick(float DeltaTime);
private:
	UCameraComponent* CameraComponent;
	CInputManager* InputManager;

	void ProcessCameraInput(float DeltaTime);
};