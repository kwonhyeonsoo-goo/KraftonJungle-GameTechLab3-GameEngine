#include "UTestObject.h"

#include "USphereMesh.h"
#include "UShader.h"
#include "Utility.h"

void UTestObject::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	SphereMesh = new USphereMesh();
	SphereMesh->CreateSphere(device);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "Color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
			D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	Shader = new UShader();
	Shader->Create(device, L"ShaderW0.hlsl", layout, ARRAYSIZE(layout), "mainVS", "mainPS");

	UseGravity = true;
}

void UTestObject::Physics_Update(float tick)
{
	ApplyGravity(tick);
	ApplyVelocity(tick);
	ApplyBoundaryCollision();
}

void UTestObject::Update(float tick)
{
	// 키 인풋에 따른 Position 변화

}

void UTestObject::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	SphereMesh->Bind(context);
	Shader->Bind(context);
	Shader->UpdateConstant(context, Position, Scale);
	SphereMesh->Draw(context);
}

void UTestObject::Release()
{
	SafeReleaseAndDelete(SphereMesh);
	SafeReleaseAndDelete(Shader);
}

void UTestObject::ApplyBoundaryCollision()
{
	if (Position.x > 1.f - Scale)
	{
		Velocity.x *= -1;
		Position.x = 1.f - Scale;
	}
	if (Position.x < -1.f + Scale)
	{
		Velocity.x *= -1;
		Position.x = -1.f + Scale;
	}
	if (Position.y > 1.f - Scale)
	{
		Velocity.y *= -1;
		Position.y = 1.f - Scale;
	}
	if (Position.y < -1.f + Scale)
	{
		Velocity.y *= -1;
		Position.y = -1.f + Scale;
	}
}
