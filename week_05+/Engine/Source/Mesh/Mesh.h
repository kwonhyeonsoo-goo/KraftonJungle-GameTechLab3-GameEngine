#pragma once
#include "CoreMinimal.h"
#include "Primitive/PrimitiveBase.h"
#include "Renderer/PrimitiveVertex.h"
#include <d3d11.h>
#include <memory>
#include <limits>
class FMaterial;


class ENGINE_API FMesh
{
public:
	FMesh() = default;
	virtual ~FMesh() = default;
	void SetMeshData(const std::shared_ptr<FMeshData>& InData) { MeshData = InData; }
	FMeshData* GetMeshData() const { return MeshData.get(); }
	const TArray<FMeshSection>& GetSections() const;
	uint32 GetNumMaterialSlots() const { return static_cast<uint32>(DefaultMaterials.size()); }

	FMaterial* GetDefaultMaterial(uint32 SlotIndex) const;
	void SetDefaultMaterial(uint32 SlotIndex, FMaterial* Mat);

	void SetAssetPath(const FString& InPath) { AssetPath = InPath; }
	const FString& GetAssetPath() const { return AssetPath; }

protected:
	std::shared_ptr<FMeshData> MeshData;
	TArray<FMaterial*> DefaultMaterials;
	FString AssetPath;
};