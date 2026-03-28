#pragma once
#include "PrimitiveComponent.h"

class FMaterial;
struct FStaticMesh;
struct ID3D11Device;

class ENGINE_API UObjComponent : public UPrimitiveComponent
{
public:
	DECLARE_RTTI(UObjComponent, UPrimitiveComponent)

	void Initialize();

	// AssetManager 기반 로딩 (StaticMesh + 멀티 머티리얼)
	void LoadStaticMeshAsset(ID3D11Device* Device, const FString& FilePath);

	FStaticMesh* GetStaticMesh() const { return StaticMesh; }
	FMaterial* GetMaterialBySlot(int32 SlotIndex) const;

private:
	// 기존 방식 소유권
	std::unique_ptr<FMaterial> DynamicMaterialOwner;

	// AssetManager 방식 (비소유, AssetManager가 수명 관리)
	FStaticMesh* StaticMesh = nullptr;
	TArray<FMaterial*> MaterialSlots;
};
