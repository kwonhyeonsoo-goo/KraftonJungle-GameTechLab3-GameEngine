#pragma once

#include "Actor.h"

class UStaticMeshComponent;
class URandomColorComponent;
struct ID3D11Device;

class ENGINE_API AStaticMeshActor : public AActor
{
public:
	DECLARE_RTTI(AStaticMeshActor, AActor)

	void PostSpawnInitialize() override;
	void LoadStaticMesh(ID3D11Device* Device, const FString& FilePath);
	UStaticMeshComponent* GetStaticMeshComponent() {	return StaticMeshComponent;}
private:
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	URandomColorComponent* RandomColorComponent = nullptr;
	bool bUseRandomColor = false;
};