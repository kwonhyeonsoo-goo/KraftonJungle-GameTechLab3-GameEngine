#include "UTexture2D.h"

#include <WICTextureLoader.h>

#include "Utility.h"

UTexture2D::UTexture2D() = default;

UTexture2D::~UTexture2D()
{
	Release();
}

bool UTexture2D::CreateFromFile(ID3D11Device* device, const std::wstring& path)
{
	if (device == nullptr)
	{
		return false;
	}

	Release();

	HRESULT hr = DirectX::CreateWICTextureFromFile(device, path.c_str(), nullptr, &ShaderResourceView);

	if (FAILED(hr))
	{
		return false;
	}

	ID3D11Resource* resource = nullptr;
	ShaderResourceView->GetResource(&resource);

	ID3D11Texture2D* texture = nullptr;
	resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture));

	if (texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);

		Width = desc.Width;
		Height = desc.Height;
		Format = desc.Format;

		texture->Release();
	}

	resource->Release();

	return SUCCEEDED(hr);
}

void UTexture2D::Bind(ID3D11DeviceContext* deviceContext, UINT slot) const
{
	if (deviceContext == nullptr)
	{
		return;
	}

	deviceContext->PSSetShaderResources(slot, 1, &ShaderResourceView);
}

void UTexture2D::Release()
{
	SafeRelease(ShaderResourceView);
}
