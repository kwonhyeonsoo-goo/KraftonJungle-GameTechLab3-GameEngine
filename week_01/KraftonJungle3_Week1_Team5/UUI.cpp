#include "UUI.h"
#include "TextureRenderer.h"
#include "Utility.h"

void UUI::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	TextureRender = new TextureRenderer();
	TextureRender->Create(device, context);
	Scale = 1.f;
}

void UUI::Physics_Update(float tick)
{

}

void UUI::Update(float tick)
{

}

void UUI::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	if (TextureRender)
	{
		TextureRender->Draw(context, device, Position, Scale);
	}
}

void UUI::Release()
{
	SafeReleaseAndDelete(TextureRender);
}

void UUI::SetTexture(const std::wstring& filePath) const
{
	if (TextureRender)
	{
		TextureRender->LoadTexture(nullptr, nullptr, filePath);
	}
}
