#pragma once
#include "SceneComponent.h"
#include "../Camera/Camera.h"

class ENGINE_API UCameraComponent : public USceneComponent
{
public:
	static UClass* StaticClass();

	UCameraComponent();
	UCameraComponent(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);
	virtual ~UCameraComponent();

	virtual void Tick(float DeltaTime) override;
	//Movement method
	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value);
	void Rotate(float DeltaYaw, float DeltaPitch);

	//Camera property getter
	CCamera* GetCamera() const;
	FMatrix GetViewMatrix() const;
	FMatrix GetProjectionMatrix() const;

	//Setting
	void SetFov(float inFov);
	void SetSpeed(float Inspeed);
private:
	CCamera* Camera = nullptr;
};