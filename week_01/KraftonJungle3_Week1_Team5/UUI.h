#pragma once
#include <string>

#include "UGameObject.h"
class TextureRenderer;

class UUI : public UGameObject
{
public:
	UUI() = default;
	~UUI() override = default;

	void Create(ID3D11Device* device, ID3D11DeviceContext* context);

	void Physics_Update(float tick) override;
	void Update(float tick) override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;
	void Release() override;

	void SetTexture(const std::wstring& filePath) const;

private:
	TextureRenderer* TextureRender;
};

