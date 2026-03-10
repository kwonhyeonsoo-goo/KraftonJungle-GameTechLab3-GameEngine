#pragma once
#include "UGameObject.h"
class UShader;
class UCubeMesh;

class UTestObject : public UGameObject
{
public:
	void Create(ID3D11Device* device, ID3D11DeviceContext* context);

	void Physics_Update(float tick) override;
	void Update(float tick) override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;
	void Release() override;

	void ApplyBoundaryCollision();
private:
	UCubeMesh* CubeMesh;
	UShader* Shader;
	float JumpForce;
	bool bOnGround;

	const float LeftBorder = -1.0f;
	const float RightBorder = 1.0f;
	const float TopBorder = 1.0f;
	const float BottomBorder = -1.0f;
};

