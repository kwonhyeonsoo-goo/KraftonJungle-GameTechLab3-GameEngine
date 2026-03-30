#pragma once

class UCameraComponent;
class FInputManager;
class USceneComponent;

class FEditorViewportController
{
public:
	~FEditorViewportController() = default;

	void Initialize(UCameraComponent* InCameraComp, FInputManager* InInput);
	void Tick(float DeltaTime);
	void SetFocus(USceneComponent* InFocusTarget);

	void SetActive(bool bInActive) { bActive = bInActive; }
	bool IsActive() const { return bActive; }

private:
	void ProcessCameraInput(float DeltaTime);

	UCameraComponent* CameraComponent = nullptr;
	FInputManager* InputManager = nullptr;

	bool bActive = false;  // false가 기본값
};