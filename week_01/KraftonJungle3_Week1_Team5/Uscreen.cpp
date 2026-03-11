#include "Uscreen.h"

#include <WICTextureLoader.h>


#include "UTextureMesh.h"
#include "UShader.h"
#include "Enum.h"

void Uscreen::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	SetObjectType(ObjectType::screen);
	Mesh = new UTextureMesh();
	Mesh->CreateRect(device);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		 { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	Shader = new UShader();
	Shader->Create(device, L"ShaderTexture.hlsl", layout, ARRAYSIZE(layout), "mainVS", "mainPS");

	UseGravity = true;
}

void Uscreen::Physics_Update(float tick)
{
	ApplyGravity(tick);
	ApplyVelocity(tick);

}

void Uscreen::Update(float tick)
{
	//아마 여기서 애니메이션 처리?

}

void Uscreen::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	Mesh->Bind(context);
	Shader->Bind(context);
	context->PSSetShaderResources(0, 1, &gTexture);
	Shader->UpdateConstant(context, Position, Scale);
	Mesh->Draw(context);
}


void Uscreen::Release()
{
}

void Uscreen::Init(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
{
	LoadTexture(Device, DeviceContext);

}

bool Uscreen::LoadTexture(ID3D11Device* device, ID3D11DeviceContext* context)
{
	HRESULT hr = DirectX::CreateWICTextureFromFile(
		device,
		context,
		filename,
		nullptr,
		&gTexture
	);

	if (FAILED(hr))
		return false;

	return true;
}