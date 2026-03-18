#pragma once

#include "ActorComponent.h"
#include <memory>

class FDynamicMaterial;
class UPrimitiveComponent;

class ENGINE_API URandomColorComponent : public UActorComponent
{
public:
	static UClass* StaticClass();

	URandomColorComponent();
	URandomColorComponent(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);
	~URandomColorComponent() override;

	void SetUpdateInterval(float InInterval) { UpdateInterval = InInterval; }
	float GetUpdateInterval() const { return UpdateInterval; }

	void BeginPlay() override;
	void Tick(float DeltaTime) override;

private:
	TObjectPtr<UPrimitiveComponent> CachedPrimitive;
	std::unique_ptr<FDynamicMaterial> DynamicMaterial;
	float UpdateInterval = 1.0f;
	float ElapsedTime = 0.0f;

	void ApplyRandomColor();
};
