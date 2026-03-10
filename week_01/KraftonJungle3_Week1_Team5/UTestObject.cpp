#include "UTestObject.h"

#include "USphereMesh.h"
#include "UShader.h"

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

	JumpForce = 1.0f;
	bOnGround = false;
}

void UTestObject::Physics_Update(float tick)
{
	ApplyGravity(tick);
	ApplyVelocity(tick);
}

void UTestObject::Update(float tick)
{
	// 2. 이동
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) Position.x -= 1.0f * tick;
	else if (GetAsyncKeyState(VK_RIGHT) & 0x8000) Position.x += 1.0f * tick;

	if (bOnGround)
	{
		if (GetAsyncKeyState(VK_UP) & 0x8000)
		{
			Velocity.y = JumpForce;
			bOnGround = false;
		}
		else if (GetAsyncKeyState(VK_LEFT) || GetAsyncKeyState(VK_RIGHT) && GetAsyncKeyState(VK_RETURN) & 0x8000)
		{
			// 다이빙
		}
	}
	else
	{
		if (GetAsyncKeyState(VK_RETURN) & 0x8000)
		{
			// 기본 스파이크 코드 

			if (GetAsyncKeyState(VK_LEFT) || GetAsyncKeyState(VK_RIGHT) & 0x8000)
			{
				// 공 충돌 코드
			}

			if (GetAsyncKeyState(VK_UP) & 0x8000)
			{
				// 공 충돌 코드
			}
			else if (GetAsyncKeyState(VK_DOWN) & 0x8000)
			{
				// 공 충돌 코드
			}
		}
	}

	if (Position.x < LeftBorder) {
		Position.x = LeftBorder;
	}
	else if (Position.x > RightBorder) {
		Position.x = RightBorder;
	}

	if (Position.y < BottomBorder) {
		Position.y = BottomBorder;
		bOnGround = true;
	}
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
}
