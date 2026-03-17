#pragma once
#include "Actor.h"
class ENGINE_API AGizmo : public AActor
{
public:
	AGizmo();

	static UClass* StaticClass();

	void PostSpawnInitialize() override;
	void BeginPlay() override;
	void Tick(float DeltaTime) override;
	void EndPlay() override;
	void Destroy() override;
};

