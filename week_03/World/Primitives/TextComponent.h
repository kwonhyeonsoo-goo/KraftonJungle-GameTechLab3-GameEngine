#pragma once
#include "World/PrimitiveComponent.h"

class UTextComponent : public UPrimitiveComponent
{
private:
	std::wstring Text = L"쿼드";
	FVector4 Color = { 1, 1, 1, 1 };

public:
	DECLARE_CLASS(UTextComponent, UPrimitiveComponent)
	UTextComponent();
	virtual void UpdateWorldAABB() override;
	bool GetRenderCommand(const FMatrix& viewMatrix, const FMatrix& projMatrix, FRenderCommand& OutCommand) override;
	bool RaycastMesh(const FRay& Ray, FHitResult& OutHitResult);
	static constexpr EPrimitiveType PrimitiveType = EPrimitiveType::EPT_Text;
	EPrimitiveType GetPrimitiveType() const override { return PrimitiveType; }

public:
	std::wstring const& GetText() const { return Text; }
	FVector4 const& GetColor() const { return Color; }

	void SetText(std::wstring& newText) { Text = newText; }
	void SetTextColor(FVector& newColor) { Color = newColor; }
};
