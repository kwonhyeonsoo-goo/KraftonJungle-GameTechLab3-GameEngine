#pragma once
#include "CoreMinimal.h"
#include "Actor.h"

class UPrimitiveComponent;
class URandomColorComponent;
class UObjComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FStaticMesh;
struct ID3D11Device;

class ENGINE_API AStaticMeshActor : public AActor
{
public:
	DECLARE_RTTI(AStaticMeshActor, AActor)

	void PostSpawnInitialize() override;

	void SetStaticMesh(UStaticMesh* InStaticMesh);

	FStaticMesh* GetStaticMeshAsset() const;

private:
	/** PostSpawnInitialize 에서 초기화 */
	UStaticMeshComponent* StaticMeshComponent = nullptr;
};