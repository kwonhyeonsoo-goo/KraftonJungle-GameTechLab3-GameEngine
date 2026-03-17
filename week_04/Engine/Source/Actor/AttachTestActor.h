#pragma once

#include "Actor.h"

class UPrimitiveComponent;

class ENGINE_API AAttachTestActor : public AActor
{
public:
	static UClass* StaticClass();

	AAttachTestActor(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);

	void PostSpawnInitialize() override;

	UPrimitiveComponent* GetSphereComponent() const { return SphereComponent; }
	UPrimitiveComponent* GetCubeComponent() const { return CubeComponent; }

private:
	UPrimitiveComponent* SphereComponent = nullptr;
	UPrimitiveComponent* CubeComponent = nullptr;
};
