#pragma once
#include "UGameObject.h"
#include "TextureRenderer.h"

class UTestObject_2 : public UGameObject
{
public:
	void Create(ID3D11Device* device, ID3D11DeviceContext* context);

	void Physics_Update(float tick) override;
	void Update(float tick) override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;
	void Release() override;

public:

	TextureRenderer* TextureRender;
	//UTextureMesh* TextureMesh;
	//UShader* Shader;
};

