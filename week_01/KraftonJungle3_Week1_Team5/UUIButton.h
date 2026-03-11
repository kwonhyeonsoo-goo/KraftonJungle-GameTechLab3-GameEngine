#pragma once
#include "UUIImage.h"

class UUIButton : public UUIImage
{
public:
	UUIButton() = default;
	~UUIButton() override = default;

	void SetSelected(bool bInSelected) { bSelected = bInSelected; }
	bool IsSelected() const { return bSelected; }
	const char* GetEditorTypeName() const override { return "UUIButton"; }

protected:
	void OnCreate(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void OnRender(ID3D11DeviceContext* context, ID3D11Device* device) override;
	void OnRelease() override;

private:
	bool bSelected = false;
};

inline void UUIButton::OnCreate(ID3D11Device* device, ID3D11DeviceContext* context)
{
	UUIImage::OnCreate(device, context);
}

inline void UUIButton::OnRender(ID3D11DeviceContext* context, ID3D11Device* device)
{
	const float OriginalScale = Scale;
	if (bSelected)
	{
		Scale *= 1.15f;
	}

	UUIImage::OnRender(context, device);
	Scale = OriginalScale;
}

inline void UUIButton::OnRelease()
{
	UUIImage::OnRelease();
}
