#pragma once

class UCameraComponent;
class FInputManager;
class USceneComponent;

class FViewportController
{
public:
	virtual ~FViewportController() = default;

	void Initialize(UCameraComponent* InCameraComp, FInputManager* InInput);
	void Tick(float DeltaTime);
	void SetFocus(USceneComponent* InFocusTarget);

	void SetActive(bool bInActive) { bActive = bInActive; }
	bool IsActive() const { return bActive; }

protected:
	virtual void ProcessCameraInput(float DeltaTime);
	virtual void KeyBoardProcess(float DeltaTime);
	virtual void MouseProcess(float DeltaTime);

	UCameraComponent* CameraComponent = nullptr;
	FInputManager* InputManager = nullptr;

	bool bActive = false;  // false가 기본값
};