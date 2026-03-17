#pragma once

#include "ActorComponent.h"

class UPrimitiveComponent;

class ENGINE_API URandomColorComponent : public UActorComponent
{
public:
	static UClass* StaticClass();

	URandomColorComponent();

	void SetUpdateInterval(float InInterval) { UpdateInterval = InInterval; }
	float GetUpdateInterval() const { return UpdateInterval; }

	void BeginPlay() override;
	void Tick(float DeltaTime) override;

private:
	UPrimitiveComponent* CachedPrimitive = nullptr;
	float UpdateInterval = 1.0f;
	float ElapsedTime = 0.0f;

	void ApplyRandomColor();
};
