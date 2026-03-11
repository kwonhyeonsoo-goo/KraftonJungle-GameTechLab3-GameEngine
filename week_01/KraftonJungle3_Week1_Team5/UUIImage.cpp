#include "UUIImage.h"

#include "TextureRenderer.h"
#include "Utility.h"

bool UUIImage::SetTexture(const std::wstring& filePath) const
{
	if (TextureRender == nullptr)
	{
		return false;
	}

	return TextureRender->LoadTexture(nullptr, nullptr, filePath);
}

void UUIImage::OnCreate(ID3D11Device* device, ID3D11DeviceContext* context)
{
	TextureRender = new TextureRenderer();
	TextureRender->Create(device, context);
}

void UUIImage::OnRender(ID3D11DeviceContext* context, ID3D11Device* device)
{
	if (TextureRender == nullptr)
	{
		return;
	}

	TextureRender->Draw(context, device, Position, Scale);
}

void UUIImage::OnRelease()
{
	SafeReleaseAndDelete(TextureRender);
}
