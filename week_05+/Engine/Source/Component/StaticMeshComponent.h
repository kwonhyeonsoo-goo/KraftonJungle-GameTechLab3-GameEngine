#pragma once
#include "CoreMinimal.h"

#include"MeshComponent.h"
class UStaticMesh;
class FArchive;
class FDynamicMaterial;
class ENGINE_API UStaticMeshComponent : public UMeshComponent
{
public:
	DECLARE_RTTI(UStaticMeshComponent, UMeshComponent)

	void Initialize();

	void LoadStaticMesh(ID3D11Device* Device, const FString& AssetName);
	FString GetStaticMeshAsset() const;

	void LoadTexture(ID3D11Device* Device, const FString& FilePath);
	void SetStaticMeshData(ID3D11Device* Device, UStaticMesh* InMesh);
	void LoadTextureToSlot(ID3D11Device* Device, const FString& FilePath, uint32 SlotIndex);
	bool IsUVScrollSupported(uint32 SlotIndex) const;
	bool IsUVScrollEnabled(uint32 SlotIndex) const;
	void SetUVScrollEnabled(uint32 SlotIndex, bool bEnabled);
	FVector2 GetUVScrollSpeed(uint32 SlotIndex) const;
	void SetUVScrollSpeed(uint32 SlotIndex, const FVector2& Speed);
	UStaticMesh* GetStaticMesh() const { return StaticMesh; }
	// UMeshComponent 인터페이스 override
	FMeshData* GetMeshData() const override;
	const TArray<FMeshSection>& GetSections() const override;
	FMaterial* GetMaterial(uint32 SlotIndex) const override;
	uint32 GetNumMaterials() const override;
	void Serialize(FArchive& Ar);
	std::shared_ptr<FDynamicMaterial> GetOrCreateDynamicMaterialForSlot(uint32 SlotIndex);
	void InitializeUVScrollParameters(uint32 SlotIndex, const std::shared_ptr<FDynamicMaterial>& DynamicMat);
	TMap<uint32, std::shared_ptr<FDynamicMaterial>> DynamicMaterialOwners;
private:
	struct FUVScrollState
	{
		bool bEnabled = false;
		FVector2 Speed = FVector2(0.0f, 0.0f);
	};



	TMap<uint32, FUVScrollState> UVScrollStates;
	UStaticMesh* StaticMesh = nullptr;
};
