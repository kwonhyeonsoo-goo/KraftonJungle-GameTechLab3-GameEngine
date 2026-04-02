#pragma once
#include "SceneComponent.h"

class FCamera;
class FArchive;
class ENGINE_API UCameraComponent : public USceneComponent
{
public:
	DECLARE_RTTI(UCameraComponent, USceneComponent)
	virtual ~UCameraComponent();

	void Initialize();
	virtual void Tick(float DeltaTime) override;
	//Movement method
	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value);
	void PanRight(float Value);
	void PanUp(float Value);
	void FootZoom(float Value);
	void Rotate(float DeltaYaw, float DeltaPitch);

	//Camera property getter
	FCamera* GetCamera() const;
	FMatrix GetViewMatrix() const;
	FMatrix GetProjectionMatrix() const;

	//Setting
	void SetFov(float inFov);
	void SetSpeed(float Inspeed);
	void SetSensitivity(float InSetSensitivity);
	void Serialize(FArchive& Ar);
private:
	FCamera* Camera = nullptr;
};