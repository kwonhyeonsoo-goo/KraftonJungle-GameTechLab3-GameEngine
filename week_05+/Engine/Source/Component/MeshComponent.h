#pragma once
#include "PrimitiveComponent.h"
#include "Primitive/PrimitiveBase.h"
#include "Math/Frustum.h"
#include <memory>
class FMaterial;
struct ID3D11Device;
struct FMeshData;
struct FMeshSection;
struct FBoxSphereBounds;
class FArchive;
class ENGINE_API UMeshComponent : public UPrimitiveComponent
{
	DECLARE_RTTI(UMeshComponent, UPrimitiveComponent)

	virtual FMeshData* GetMeshData() const { return nullptr; }
	virtual const TArray<FMeshSection>& GetSections() const;
	virtual uint32                    GetNumMaterials() const { return 0; }


	virtual FMaterial* GetMaterial(uint32 SlotIndex) const;
	void SetMaterial(uint32 SlotIndex, FMaterial* Mat);


	virtual FBoxSphereBounds GetWorldBounds() const;
	virtual void Serialize(FArchive& Ar) override;
protected:

	TArray<FMaterial*> OverrideMaterials;
};