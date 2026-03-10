#pragma once
#include <d3d11.h>

#include "UResource.h"
class UTexture2D : public UResource
{
public:
	UTexture2D();
	~UTexture2D() override;

	bool CreateFromFile(ID3D11Device* device, const std::wstring& path);
	void Bind(ID3D11DeviceContext* deviceContext, UINT slot = 0) const;
	void Release();

	ID3D11ShaderResourceView* GetSRV() const { return ShaderResourceView; }
	EResourceType GetType() override { return EResourceType::Texture2D; }

	uint32_t GetWidth() const { return Width; }
	uint32_t GetHeight() const { return Height; }

private:
	ID3D11ShaderResourceView* ShaderResourceView = nullptr;
	uint32_t Width = 0;
	uint32_t Height = 0;
	DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
};

