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
	if (Texture == nullptr || Mesh == nullptr || Shader == nullptr || context == nullptr || device == nullptr)
	{
		return;
	}
	float flipScale = isFlipDraw ? -1.0f*Scale : 1.0f* Scale;
	UpdateMeshForCurrentViewport(device, context);

	ID3D11ShaderResourceView* srv = Texture->GetSRV();

	Mesh->Bind(context);
	Shader->Bind(context);
	context->PSSetShaderResources(0, 1, &srv);
	Shader->UpdateConstant(context, Position, flipScale);
	Mesh->Draw(context);
}

void TextureRenderer::Init(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, const std::wstring& filePath)
{
	LoadTexture(Device, DeviceContext, filePath);
}

bool TextureRenderer::LoadTexture(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, const std::wstring& filePath)
{
	Texture = UEngine::GetInstance().GetResourceManager().LoadTexture(filePath);
	bMeshNeedsUpdate = true;

	return Texture != nullptr;
}

void TextureRenderer::Release()
{
	SafeReleaseAndDelete(Mesh);
	SafeReleaseAndDelete(Shader);
	CachedViewportWidth = 0.0f;
	CachedViewportHeight = 0.0f;
	bMeshNeedsUpdate = true;
}

bool TextureRenderer::UpdateMeshForCurrentViewport(ID3D11Device* device, ID3D11DeviceContext* context)
{
	if (device == nullptr || context == nullptr || Texture == nullptr || Mesh == nullptr)
	{
		return false;
	}

	UINT viewportCount = 0;
	context->RSGetViewports(&viewportCount, nullptr);
	if (viewportCount == 0)
	{
		return false;
	}

	D3D11_VIEWPORT viewport = {};
	viewportCount = 1;
	context->RSGetViewports(&viewportCount, &viewport);
	if (viewport.Width <= 0.0f || viewport.Height <= 0.0f)
	{
		return false;
	}

	if (!bMeshNeedsUpdate &&
		CachedViewportWidth == viewport.Width &&
		CachedViewportHeight == viewport.Height)
	{
		return true;
	}

	const float halfWidthNdc = static_cast<float>(Texture->GetWidth()) / viewport.Width;
	const float halfHeightNdc = static_cast<float>(Texture->GetHeight()) / viewport.Height;

	if (!Mesh->CreateRect(device, halfWidthNdc, halfHeightNdc))
	{
		return false;
	}

	CachedViewportWidth = viewport.Width;
	CachedViewportHeight = viewport.Height;
	bMeshNeedsUpdate = false;
	return true;
}

void TextureRenderer::SetTexture(UTexture2D* newTexture) {
	Texture = newTexture; // 찾을 필요 없이 바로 대입! (초고속)
	bMeshNeedsUpdate = true;  //
}