#pragma once

#include "Actor.h"


class UStaticMeshComponent;
class ENGINE_API ASkySphereActor : public AActor
{
public:
	DECLARE_RTTI(ASkySphereActor, AActor)
	void PostSpawnInitialize() override;


	void Tick(float DeltaTime) override;
	void LoadSkyMesh(ID3D11Device* Device);
	UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }

private:
	UStaticMeshComponent* StaticMeshComponent = nullptr;
};
