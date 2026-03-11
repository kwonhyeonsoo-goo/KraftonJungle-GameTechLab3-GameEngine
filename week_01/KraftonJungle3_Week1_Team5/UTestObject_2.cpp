#include "UTestObject_2.h"

#include "TextureRenderer.h"
#include "Animator.h"
#include "UShader.h"


#pragma region AnimationFrames


#pragma endregion

void UTestObject_2::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// Ensure TextureRender is allocated before calling its methods

	/*	SphereMesh = new USphereMesh();
	SphereMesh->CreateSphere(device);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "Color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
			D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	Shader = new UShader();
	Shader->Create(device, L"ShaderW0.hlsl", layout, ARRAYSIZE(layout), "mainVS", "mainPS");*/

	TextureRender = new TextureRenderer();
	TextureRender->Create(device, context);
	TextureRender->Init(device, context, L"sprite_sheet.png");


	AnimatorComponent = new Animator();
	AnimatorComponent->SetFrameDuration(0.1f);

	std::vector<std::wstring> idleFrames = {
	L"Resources/Textures/pikachu/pikachu_0_0.png",
	L"Resources/Textures/pikachu/pikachu_0_1.png",
	L"Resources/Textures/pikachu/pikachu_0_2.png",
	L"Resources/Textures/pikachu/pikachu_0_3.png",
	L"Resources/Textures/pikachu/pikachu_0_4.png"
	};
	AnimatorComponent->AddFrames("Idle", idleFrames);

	UseGravity = true;
}

void UTestObject_2::Physics_Update(float tick)
{
	ApplyGravity(tick);
	ApplyVelocity(tick);

}

void UTestObject_2::Update(float tick)
{
	//amimation update
	AnimatorComponent->Play(TextureRender, "Idle", tick);

}

void UTestObject_2::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	TextureRender->SetTexture(AnimatorComponent->GetCurrentFrame());

	TextureRender->Draw(context, device, Position, Scale);

}

void UTestObject_2::Release()
{
	if (TextureRender)
	{
		delete TextureRender;
		TextureRender = nullptr;
	}

	if (AnimatorComponent)
	{
		delete AnimatorComponent;
		AnimatorComponent = nullptr;
	}
}
