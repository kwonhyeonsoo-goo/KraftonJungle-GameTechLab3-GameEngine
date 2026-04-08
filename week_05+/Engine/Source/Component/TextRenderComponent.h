#pragma once
#include "PrimitiveComponent.h"
class FArchive;
class ENGINE_API UTextRenderComponent : public UPrimitiveComponent
{
public:
	DECLARE_RTTI(UTextRenderComponent, UPrimitiveComponent)

	virtual void Initialize();
	virtual FBoxSphereBounds GetWorldBounds() const override;
	void Serialize(FArchive& Ar);

	/**표시할 텍스트 설정 - 메시 데이터가 갱신될 수 있도록 유도함 */
	void SetText(const FString& InText);
	const FString& GetText() const { return Text; }



	void SetTextColor(const FVector4& InColor) { TextColor = InColor; }
	const FVector4& GetTextColor() const { return TextColor; }

	void SetBillboard(bool bInBillboard) { bBillboard = bInBillboard; }
	bool IsBillboard() const { return bBillboard; }

	void SetTextScale(float InScale) { TextScale = InScale; }
	float GetTextScale() const { return TextScale; }

	/** 기존 UUIDBillboardComponent와의 호환성 및 편의를 위해 추가 */
	void SetWorldScale(float InScale) { TextScale = InScale; }
	float GetWorldScale() const { return TextScale; }

	virtual FString GetDisplayText() const;
	virtual FVector GetRenderWorldPosition() const { return GetWorldLocation(); }
	virtual FVector GetRenderWorldScale() const { return GetWorldTransform().GetScaleVector() * TextScale; }

	const FVector& GetWorldOffset() const { return WorldOffset; }
	void SetWorldOffset(const FVector& InOffset) { WorldOffset = InOffset; }

	struct FMeshData* GetTextMesh() const { return TextMesh.get(); }

protected:
	FString Text = "";
	FVector4 TextColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	float TextScale = 1.0f;
	bool bBillboard = false;
	FVector WorldOffset = FVector(0.0f, 0.0f, 0.3f);

	std::shared_ptr<struct FMeshData> TextMesh;
};