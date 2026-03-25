#pragma once
#include "PrimitiveComponent.h"

class ENGINE_API UTextComponent : public UPrimitiveComponent
{
public:
	DECLARE_RTTI(UTextComponent, UPrimitiveComponent)

	void Initialize();

	virtual FBoxSphereBounds GetWorldBounds() const override;

	/** 표시할 텍스트 설정 - 메시 데이터가 갱신될 수 있도록 유도함 */
	void SetText(const FString& InText);
	const FString& GetText() const { return Text; }

	void SetTextColor(const FVector4& InColor) { TextColor = InColor; }
	const FVector4& GetTextColor() const { return TextColor; }

	void SetBillboard(bool bInBillboard) { bBillboard = bInBillboard; }
	bool IsBillboard() const { return bBillboard; }

	/** 폰트 렌더링용 메시 데이터 반환 */
	struct FMeshData* GetTextMesh() const { return TextMesh.get(); }

private:
	FString Text = "Text";
	FVector4 TextColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	bool bBillboard = false;

	/** 텍스트 렌더링을 위해 생성된 동적 메시 데이터 */
	std::shared_ptr<struct FMeshData> TextMesh;
};