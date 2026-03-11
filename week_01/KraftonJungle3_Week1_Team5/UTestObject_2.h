#pragma once
#include "UGameObject.h"
#include "TextureRenderer.h"
#include "Animator.h"


class UTestObject_2 : public UGameObject
{
public:
	void Create(ID3D11Device* device, ID3D11DeviceContext* context);

	void Physics_Update(float tick) override;
	void Update(float tick) override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;
	const char* GetEditorTypeName() const override { return "UTestObject_2"; }
	void Release() override;

public:

	TextureRenderer* TextureRender;
	Animator* AnimatorComponent;
	//UTextureMesh* TextureMesh;
	//UShader* Shader;
};

