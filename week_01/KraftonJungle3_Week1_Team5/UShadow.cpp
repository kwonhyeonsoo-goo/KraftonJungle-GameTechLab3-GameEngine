#include "UShadow.h"

#include "TextureRenderer.h"
#include "Utility.h"

UShadow::UShadow() = default;

UShadow::~UShadow() = default;

UShadow* UShadow::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	UShadow* instance = new UShadow();
	instance->TextureRender = new TextureRenderer();
	instance->TextureRender->Create(device, context);
	instance->TextureRender->LoadTexture(device, context, L"Resource\\Image\\objects\\shadow.png");

	instance->Scale = 1.f;

	return instance;
}

void UShadow::Physics_Update(float tick)
{
}

void UShadow::Update(float tick)
{
	if (Target)
	{
		FVector3 targetPos = Target->GetPosition();
		SetPosition({ targetPos.x, ShadowYPos, 0.f });
	}
}

void UShadow::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	if (TextureRender)
	{
		TextureRender->Draw(context, device, Position, Scale);
	}
}

void UShadow::Release()
{
	SafeReleaseAndDelete(TextureRender);
}
