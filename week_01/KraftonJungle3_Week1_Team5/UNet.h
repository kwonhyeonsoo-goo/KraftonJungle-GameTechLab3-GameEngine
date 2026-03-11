#pragma once
#include "UGameObject.h"
#include "UBall.h"

class UCubeMesh;
class UShader;

class UNet : public UGameObject
{

public:
	void Create(ID3D11Device* device, ID3D11DeviceContext* context);
	void Physics_Update(float tick) override;
	void Update(float tick) override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;
	void Release() override;

	void HandleBallCollision(UBall* ball);

private:
    static constexpr float HalfWidth  = 0.001f;  // 네트 반너비
    static constexpr float HalfHeight = 0.35f;   // 네트 반높이

    UCubeMesh* CubeMesh = nullptr;
    UShader*   Shader   = nullptr;

};

