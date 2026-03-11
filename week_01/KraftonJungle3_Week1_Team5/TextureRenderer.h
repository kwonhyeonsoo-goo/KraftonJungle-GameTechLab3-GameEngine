#pragma once
#include <d3d11.h>
#include <string>

#include "FVector3.h"

class UTexture2D;
class UShader;
class UTextureMesh;

class TextureRenderer
{
public:
	TextureRenderer();
	~TextureRenderer();

	void Create(ID3D11Device* device, ID3D11DeviceContext* context); //메시와 세이더 생성
	void Draw(ID3D11DeviceContext* context, ID3D11Device* device, FVector3 Position, float Scale);
	void Init(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, const std::wstring& filePath);
	bool LoadTexture(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, const std::wstring& filePath);
	void SetTexture(UTexture2D* newTexture);
	void SetFlipDraw(bool bFlip) { isFlipDraw = bFlip; }
	bool GetFlipDraw() { return isFlipDraw; }
	void Release();

private:
	bool UpdateMeshForCurrentViewport(ID3D11Device* device, ID3D11DeviceContext* context);

private:
	UTexture2D* Texture = nullptr;
	UTextureMesh* Mesh;
	UShader* Shader;
	float CachedViewportWidth = 0.0f;
	float CachedViewportHeight = 0.0f;
	bool bMeshNeedsUpdate = true;
	bool isFlipDraw=false;
};

