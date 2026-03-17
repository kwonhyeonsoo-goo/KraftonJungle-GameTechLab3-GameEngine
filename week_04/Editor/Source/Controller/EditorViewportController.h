#pragma once

class UCameraComponent;
class CInputManager;

class CEditorCViewportController
{
public:
	CEditorCViewportController();
	void Tick(float DeltaTime);
private:
	UCameraComponent* CameraComp;
	CInputManager* InputManager;
};