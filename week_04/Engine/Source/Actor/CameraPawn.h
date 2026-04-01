#pragma once

#pragma once
#include "CoreMinimal.h"

#include "Actor/Actor.h"

class UCameraComponent;


class ENGINE_API UCameraPawn : public AActor
{
public:
	DECLARE_RTTI(UCameraPawn, AActor)
	void Initialize();
	virtual void PostSpawnInitialize() override;
	UCameraComponent* GetCameraComponent() const { return CameraCompenent; }

private:
	UCameraComponent* CameraCompenent = nullptr;
};
