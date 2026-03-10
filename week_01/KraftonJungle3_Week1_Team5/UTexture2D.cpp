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
