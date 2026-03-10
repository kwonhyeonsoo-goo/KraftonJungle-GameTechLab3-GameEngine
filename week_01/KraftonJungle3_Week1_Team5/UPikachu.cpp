#include "UPikachu.h"

#include "UCubeMesh.h"
#include "UShader.h"
#include "USphereMesh.h"
#include "Utility.h"

void UPikachu::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	CubeMesh = new UCubeMesh();
	CubeMesh->CreateCube(device);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "Color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
			D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	Shader = new UShader();
	Shader->Create(device, L"ShaderW0.hlsl", layout, ARRAYSIZE(layout), "mainVS", "mainPS");

	UseGravity = true;

	JumpForce = 3.0f;
	bOnGround = false;
}

void UPikachu::Physics_Update(float tick)
{
	// 속도 (Velocity)을 위치에 반영한 후에 -> 입력을 통한 움직임 같은거 -> 모든 움직임 처리 후에 
	// 충돌 처리를 해주고 그 충돌에 따른 물리 반영
	Move(tick);
	ApplyGravity(tick);
	ApplyVelocity(tick);
	ApplyBoundaryCollision();
}

void UPikachu::Update(float tick)
{
}

void UPikachu::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	CubeMesh->Bind(context);
	Shader->Bind(context);
	Shader->UpdateConstant(context, Position, Scale);
	CubeMesh->Draw(context);
}

void UPikachu::Release()
{
	SafeReleaseAndDelete(CubeMesh);
	SafeReleaseAndDelete(Shader);
}

void UPikachu::ApplyBoundaryCollision()
{
	if (Position.x > 1.f - Scale)
	{
		Velocity.x = 0;
		Position.x = 1.f - Scale;
	}
	if (Position.x < -1.f + Scale)
	{
		Velocity.x = 0;
		Position.x = -1.f + Scale;
	}
	if (Position.y > 1.f - Scale)
	{
		Velocity.y = 0;
		Position.y = 1.f - Scale;
	}
	if (Position.y < -1.f + Scale)
	{
		bOnGround = true;
		Velocity.y = 0;
		Position.y = -1.f + Scale;

		if (CurrentState == EPlayerState::Diving)
		{
			CurrentState = EPlayerState::Recovering;
			Velocity.x = 0.0f;
			RecoveryTimer = 0.5f;
		}
	}
}

void UPikachu::Move(float tick)
{
	if (CurrentState == EPlayerState::Recovering)
	{
		RecoveryTimer -= tick;
		if (RecoveryTimer <= 0.0f)
		{
			CurrentState = EPlayerState::Normal;
		}
		else
		{
			return;
		}
	}
	else if (CurrentState == EPlayerState::Diving)
	{
		return;
	}

	CurrentState = EPlayerState::Normal;

	int currentInput = FLAG_NONE;
	if (GetAsyncKeyState(VK_RETURN) & 0x8000) currentInput |= FLAG_SPIKE;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) currentInput |= FLAG_LEFT;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) currentInput |= FLAG_RIGHT;
	if (GetAsyncKeyState(VK_UP) & 0x8000) currentInput |= FLAG_UP;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000) currentInput |= FLAG_DOWN;

	// 2. 이동
	if (currentInput & FLAG_LEFT) Position.x -= 1.0f * tick;
	if (currentInput & FLAG_RIGHT) Position.x += 1.0f * tick;

	if (bOnGround)
	{
		if (currentInput & FLAG_UP)
		{
			Velocity.y = JumpForce;
			bOnGround = false;
		}
		else if (currentInput & FLAG_SPIKE)
		{
			if (currentInput & FLAG_LEFT)
			{
				CurrentState = EPlayerState::Diving;
				Velocity.x = -1.1f;
				Velocity.y = JumpForce * 0.3f;
				bOnGround = false;
			}
			else if (currentInput & FLAG_RIGHT)
			{
				CurrentState = EPlayerState::Diving;
				Velocity.x = 1.1f;
				Velocity.y = JumpForce * 0.3f;
				bOnGround = false;
			}
		}
	}
	else
	{
		if (currentInput & FLAG_SPIKE)
		{
			CurrentState = GetSpikeStateFromInput(currentInput);
		}
	}
}

EPlayerState UPikachu::GetSpikeStateFromInput(int input)
{
	static EPlayerState SpikeStateLUT[32];
	static bool bInitialized = false;

	if (!bInitialized)
	{
		for (int i = 0; i < 32; ++i)
		{
			SpikeStateLUT[i] = EPlayerState::BasicSpike;
		}

		SpikeStateLUT[FLAG_SPIKE] = EPlayerState::BasicSpike;
		SpikeStateLUT[FLAG_SPIKE | FLAG_LEFT] = EPlayerState::FrontSpike;
		SpikeStateLUT[FLAG_SPIKE | FLAG_RIGHT] = EPlayerState::FrontSpike;
		SpikeStateLUT[FLAG_SPIKE | FLAG_UP] = EPlayerState::UpSpike;
		SpikeStateLUT[FLAG_SPIKE | FLAG_DOWN] = EPlayerState::DownSpike;

		SpikeStateLUT[FLAG_SPIKE | FLAG_UP | FLAG_LEFT] = EPlayerState::UpFrontSpike;
		SpikeStateLUT[FLAG_SPIKE | FLAG_UP | FLAG_RIGHT] = EPlayerState::UpFrontSpike;
		SpikeStateLUT[FLAG_SPIKE | FLAG_DOWN | FLAG_LEFT] = EPlayerState::DownFrontSpike;
		SpikeStateLUT[FLAG_SPIKE | FLAG_DOWN | FLAG_RIGHT] = EPlayerState::DownFrontSpike;

		bInitialized = true;
	}

	return SpikeStateLUT[input];
}