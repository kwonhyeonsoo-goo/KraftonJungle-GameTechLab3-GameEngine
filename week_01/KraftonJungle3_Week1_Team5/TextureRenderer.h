#pragma once
#include "WICTextureLoader.h"
#include "FVector3.h"

class UShader;
class UTextureMesh;


class TextureRenderer
{
public:
	UTextureMesh* Mesh;
	UShader* Shader;

	void Create(ID3D11Device* device, ID3D11DeviceContext* context); //메시와 세이더 생성
	void Draw(ID3D11DeviceContext* context, ID3D11Device* device, FVector3 Position, float Scale);
	void Init(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, const wchar_t* flnm);
	bool LoadTexture(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, const wchar_t* flnm);

private:
	ID3D11ShaderResourceView* gTexture = nullptr;

};

