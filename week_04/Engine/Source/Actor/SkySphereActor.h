#pragma once

#include "Actor.h"


class USkyComponent;
class ENGINE_API ASkySphereActor : public AActor
{
public:
	static UClass* StaticClass();

	ASkySphereActor(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);

	void PostSpawnInitialize() override;
	void Tick(float DeltaTime) override;
	USkyComponent* GetSkyComponent() const { return SkyComponent; }

private:
	USkyComponent* SkyComponent = nullptr;
};
