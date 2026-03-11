#include "UNet.h"
#include "UCubeMesh.h"
#include "UShader.h"
#include "Utility.h"

void UNet::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// 네트 중심: x=0, y = 바닥(-1) + 반높이(0.35) = -0.65
	Position = { 0.f, -0.65f, 0.f };

	CubeMesh = new UCubeMesh();
	CubeMesh->CreateCube(device);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "Color",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	Shader = new UShader();
	Shader->Create(device, L"ShaderW0.hlsl", layout, ARRAYSIZE(layout), "mainVS", "mainPS");

	UseGravity = false;
}

void UNet::Physics_Update(float tick)
{
	
}

void UNet::Update(float tick)
{
	
}

void UNet::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	CubeMesh->Bind(context);
	Shader->Bind(context);
	Shader->UpdateConstant(context, Position, HalfHeight); // 높이 기준으로 렌더
	CubeMesh->Draw(context);
}

void UNet::Release()
{
	SafeReleaseAndDelete(CubeMesh);
	SafeReleaseAndDelete(Shader);
}

void UNet::HandleBallCollision(UBall* ball)
{
	FVector3 BallPos = ball->GetPosition();
	float r = ball->GetRadius();

	float netLeft = Position.x - HalfWidth; // -0.001f
	float netRight = Position.x + HalfWidth;//  0.015f
	float netTop = Position.y + HalfHeight + 0.075f;   // -0.65 + 0.35 = -0.30
	float netBottom = -1.0f;

	// AABB vs Circle
	bool xOverlap = (BallPos.x + r > netLeft) && (BallPos.x - r < netRight);
	bool yOverlap = (BallPos.y + r > netBottom) && (BallPos.y - r < netTop);
	if (!xOverlap || !yOverlap) return;

	float distX = fabsf(BallPos.x - Position.x); // 좌우 대칭이라 절댓값 하나로 처리
	float xPen = (HalfWidth - distX) + r;
	float yPen = (netTop - BallPos.y) + r;

	FVector3 vel = ball->GetVelocity();

	if (xPen < yPen)
	{
		// 측면 충돌
		float sign = (BallPos.x >= Position.x) ? 1.f : -1.f;
		if (vel.x * sign < 0.f)
		{
			ball->SetVelocity({ -vel.x, vel.y, 0.f });
			ball->SetPosition({ BallPos.x + sign * xPen, BallPos.y, BallPos.z });
		}
	}
	else
	{
		// 상단 충돌
		if (vel.y < 0.f)
		{
			ball->SetVelocity({ vel.x, -vel.y, 0.f });
			ball->SetPosition({ BallPos.x, netTop + r, BallPos.z });
		}
	}
}
