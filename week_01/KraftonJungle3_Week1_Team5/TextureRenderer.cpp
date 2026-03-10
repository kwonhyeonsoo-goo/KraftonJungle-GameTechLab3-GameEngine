#include "TextureRenderer.h"

#include <WICTextureLoader.h>

#include "UEngine.h"
#include "UTextureMesh.h"
#include "UShader.h"
#include "Utility.h"

TextureRenderer::TextureRenderer() : Mesh(nullptr), Shader(nullptr)
{
}

TextureRenderer::~TextureRenderer()
{
	Release();
}

void TextureRenderer::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	Mesh = new UTextureMesh();
	Mesh->CreateRect(device);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		 { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	Shader = new UShader();
	Shader->Create(device, L"ShaderTexture.hlsl", layout, ARRAYSIZE(layout), "mainVS", "mainPS");

}

void TextureRenderer::Draw(ID3D11DeviceContext* context, ID3D11Device* device, FVector3 Position, float Scale)
{
	ID3D11ShaderResourceView* srv = Texture->GetSRV();

	Mesh->Bind(context);
	Shader->Bind(context);
	context->PSSetShaderResources(0, 1, &srv);
	Shader->UpdateConstant(context, Position, Scale);
	Mesh->Draw(context);
}

void TextureRenderer::Init(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, const std::wstring& filePath)
{
	LoadTexture(Device, DeviceContext, filePath);
}

bool TextureRenderer::LoadTexture(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, const std::wstring& filePath)
{
	Texture = UEngine::GetInstance().GetResourceManager().LoadTexture(filePath);

	return true;
}

void TextureRenderer::Release()
{
	SafeReleaseAndDelete(Mesh);
	SafeReleaseAndDelete(Shader);
}
