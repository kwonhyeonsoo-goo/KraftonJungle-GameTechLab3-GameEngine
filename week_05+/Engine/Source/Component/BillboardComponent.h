#pragma once
#include "Component/PrimitiveComponent.h"
#include "Math/Vector2.h"
#include <memory>

struct FMeshData;

class ENGINE_API UBillboardComponent : public UPrimitiveComponent
{
public:
	DECLARE_RTTI(UBillboardComponent, UPrimitiveComponent)

	void Initialize();

	virtual FBoxSphereBounds GetWorldBounds() const override;

	void SetSize(const FVector2& InSize) { 
		Size = InSize; 
		MarkTransformDirty();
	}
	const FVector2& GetSize() const { return Size; }

	FMeshData* GetBillboardMesh() const { return BillboardMesh.get(); }
	virtual void Serialize(class FArchive& Ar) override;

	const FString& GetTexturePath() const { return TexturePath; }
	void SetTexturePath(ID3D11Device* Device, const FString& InPath);
private:

	FVector2 Size = FVector2(1.0f, 1.0f);
	std::shared_ptr<FMeshData> BillboardMesh;
	FString TexturePath = "";

	std::shared_ptr<class FDynamicMaterial> DynamicMaterial;
};