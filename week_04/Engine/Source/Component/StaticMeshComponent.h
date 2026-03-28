#pragma once
#include "MeshComponent.h"
#include "../ThirdParty/nlohmann/json.hpp"
#include "Types/CoreTypes.h"

class UStaticMesh;
class FMaterial;
class FArchive;
struct FMeshData;

class ENGINE_API UStaticMeshComponent : public UMeshComponent
{
public:
	DECLARE_RTTI(UStaticMeshComponent, UMeshComponent)

	void SetStaticMesh(UStaticMesh* InMesh);
	UStaticMesh* GetStaticMesh() const { return StaticMesh; }

	FMaterial* GetMaterial(int32 SlotIndex) const { return OverrideMaterials[SlotIndex]; }
	void SetMaterial(int32 SlotIndex, FMaterial* InMat) { OverrideMaterials[SlotIndex] = InMat; }

	FBoxSphereBounds GetWorldBounds() const override;
	FMeshData* GetMeshData() const;

	void Serialize(FArchive& Ar) override {};
private:
	UStaticMesh* StaticMesh = nullptr;
	// 런타임 Override 가능
	TArray<FMaterial*> OverrideMaterials;
};
