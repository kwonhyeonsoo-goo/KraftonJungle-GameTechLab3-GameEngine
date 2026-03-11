#include "UUIScore.h"

#include "TextureRenderer.h"
#include "UEngine.h"

void UUIScore::OnCreate(ID3D11Device* device, ID3D11DeviceContext* context)
{
	UUIImage::OnCreate(device, context);

	std::wstring baseImagePath = L"Resource\\Image\\number\\number_";

	Numbers.reserve(10);

	for (int i = 0; i < 10; ++i)
	{
		std::wstring path = baseImagePath + std::to_wstring(i) + L".png";
		Numbers.push_back(UEngine::GetInstance().GetResourceManager().LoadTexture(path));
	}

}

void UUIScore::OnUpdate(float tick)
{
	UUIImage::OnUpdate(tick);
	SetTexture(Numbers[Score]);
}
