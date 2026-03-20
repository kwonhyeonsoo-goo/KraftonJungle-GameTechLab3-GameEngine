
#pragma once
#include "Actor/Actor.h"
#include "Component/CameraComponent.h"

class AEditorCameraPawn : public AActor
{
public:
	DECLARE_RTTI(AEditorCameraPawn, AActor)
	void Initialize();

	UCameraComponent* GetCameraComponent() const { return CameraCompenent; }

private:
	UCameraComponent* CameraCompenent = nullptr;
};
