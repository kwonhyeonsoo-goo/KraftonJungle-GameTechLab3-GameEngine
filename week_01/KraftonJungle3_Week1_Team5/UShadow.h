#pragma once
#include "UGameObject.h"
class TextureRenderer;

class UShadow : public UGameObject
{
public:
	UShadow();
	~UShadow() override;

	static UShadow* Create(ID3D11Device* device, ID3D11DeviceContext* context);
	
	void Physics_Update(float tick) override;
	void Update(float tick) override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;
	void Release() override;

	const char* GetEditorTypeName() const override { return "UShadow"; }

	void SetTarget(UGameObject* target) { Target = target; }

private:
	TextureRenderer* TextureRender = nullptr;
	UGameObject* Target = nullptr;

	static constexpr float ShadowYPos = -0.808f;
};

