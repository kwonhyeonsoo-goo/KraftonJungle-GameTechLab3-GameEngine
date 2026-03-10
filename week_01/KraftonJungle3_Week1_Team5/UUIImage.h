#pragma once
#include <string>

#include "UUI.h"

class TextureRenderer;

class UUIImage : public UUI
{
public:
	UUIImage() = default;
	~UUIImage() override = default;

	bool SetTexture(const std::wstring& filePath) const;
	const char* GetEditorTypeName() const override { return "UUIImage"; }

protected:
	void OnCreate(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void OnRender(ID3D11DeviceContext* context, ID3D11Device* device) override;
	void OnRelease() override;

private:
	TextureRenderer* TextureRender = nullptr;
};
