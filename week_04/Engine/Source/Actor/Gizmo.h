#pragma once
#include "Actor.h"
class ENGINE_API AGizmo : public AActor
{
public:
	DECLARE_RTTI(AGizmo, AActor)

	void PostSpawnInitialize() override;
	void BeginPlay() override;
	void Tick(float DeltaTime) override;
	void EndPlay() override;
	void Destroy() override;
};

