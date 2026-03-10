#pragma once
#include "UUIImage.h"
class UUIButton : public UUIImage
{
public:
	UUIButton() = default;
	~UUIButton() override = default;

protected:
	void OnCreate(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void OnRender(ID3D11DeviceContext* context, ID3D11Device* device) override;
	void OnRelease() override;

private:
	bool bSelected = false;
};
