#include "UPikachu.h"

#include "UCubeMesh.h"
#include "USphereMesh.h"
#include "UShader.h"
#include "Utility.h"
#include "Enum.h"
#include "UBall.h"
#include <algorithm>

void UPikachu::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	SetObjectType(ObjectType::Pikachu);

	/*CubeMesh = new UCubeMesh();
	CubeMesh->CreateCube(device);*/
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

	JumpForce = 3.0f;
	bOnGround = false;
	RecoveryTimer = 0.0f;
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
	SphereMesh->Bind(context);
	Shader->Bind(context);
	Shader->UpdateConstant(context, Position, Scale);
	SphereMesh->Draw(context);
}

void UPikachu::HandleCollision(UBall* ball)
{
	FVector3 BallPos = ball->GetPosition();
	FVector3 BTOP = (BallPos - Position).Normalize();

	// 위치 보정
	if (CurrentState == EPlayerState::Normal)
	{
		float dist = (BallPos - Position).Length();
		float overlap = (ball->GetRadius() + Scale) - dist;
		ball->SetPosition(BallPos + BTOP * overlap);
	}

	// 상대 속도 체크: 이미 멀어지는 중이면 스킵
	// 충돌했을때 겹치는 부분이 매우 크다면? -> 
	FVector3 relativeVelocity = ball->GetVelocity() - Velocity;
	float closingSpeed = FVector3::DotProduct(BTOP, relativeVelocity);
	if (closingSpeed > 0) return;

	float xDiff = BallPos.x - Position.x;
	float maxDist = ball->GetRadius() + Scale;
	float newXVel = (xDiff / maxDist) * 2.0f;

	// 공 y속도 + 플레이어 점프 속도 합산
	// 절댓값으로 무조건 공이 플레이어보다 올라가게
	float newYVel = fabsf(ball->GetVelocity().y) + max(0.f, Velocity.y);
	if (newYVel < 1.5f) newYVel = 1.5f;
	if (BallPos.y < Position.y) newYVel *= -1;
	

	

	// 플레이어가 왼쪽, 오른쪽인지에 따라 방향 구분
	float xSign = (Position.x <= -0.02f && Position.x >= -1.0f) ? 1.f : -1.f;

	switch (CurrentState)
	{
	case EPlayerState::BasicSpike:
		// 앞으로 적당하게
		newXVel = xSign * 2.0f; // 수정해야함.
		newYVel = 0.5f;
		break;

	case EPlayerState::FrontSpike:
		// 옆
		newXVel = xSign * 4.0f;
		newYVel = 2.0f;
		break;

	case EPlayerState::UpSpike:
		// 위
		newXVel *= 0.5f;
		newYVel = 5.0f;
		break;

	case EPlayerState::DownSpike:
		// 아
		newXVel = xSign * 2.f;
		newYVel = -4.0f;
		break;

	case EPlayerState::UpFrontSpike:
		// 위 + 앞 대각선
		newXVel = xSign * 3.0f;
		newYVel = 4.0f;
		break;

	case EPlayerState::DownFrontSpike:
		// 앞 + 아래 대각선
		newXVel = xSign * 3.5f;
		newYVel = -3.0f;
		break;

	default: // Normal
		newYVel = min(newYVel, 3.0f);
		break;
	}

	// 속력 제한
	const float MaxBallSpeed = 4.0f;
	const float MinBallSpeed = 2.0f;
	float speed = sqrtf(newXVel * newXVel + newYVel * newYVel);

	if (speed > 0.f)
	{
		float clampedSpeed = max(MinBallSpeed, min(speed, MaxBallSpeed));
		float ratio = clampedSpeed / speed;
		newXVel *= ratio;
		newYVel *= ratio;
	}

	ball->SetVelocity({ newXVel, newYVel, 0.f });
}

void UPikachu::Release()
{
	SafeReleaseAndDelete(SphereMesh);
	SafeReleaseAndDelete(Shader);
}

void UPikachu::SetBoundary(float left, float right, float top, float bottom)
{
	LeftBorder = left;
	RightBorder = right;
	TopBorder = top;
	BottomBorder = bottom;
}

void UPikachu::ApplyBoundaryCollision()
{
	if (Position.x > RightBorder - Scale)
	{
		Velocity.x = 0;
		Position.x = RightBorder - Scale;
	}
	if (Position.x < LeftBorder + Scale)
	{
		Velocity.x = 0;
		Position.x = LeftBorder + Scale;
	}
	if (Position.y > TopBorder - Scale)
	{
		Velocity.y = 0;
		Position.y = TopBorder - Scale;
	}
	if (Position.y < BottomBorder + Scale)
	{
		bOnGround = true;
		Velocity.y = 0;
		Position.y = BottomBorder + Scale;

		if (CurrentState == EPlayerState::Diving)
		{
			CurrentState = EPlayerState::Recovering;
			Velocity.x = 0.0f;
			RecoveryTimer = 0.25f;
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
	if (GetAsyncKeyState(KeyConfig.SpikeKey) & 0x8000) currentInput |= FLAG_SPIKE;
	if (GetAsyncKeyState(KeyConfig.LeftKey) & 0x8000) currentInput |= FLAG_LEFT;
	if (GetAsyncKeyState(KeyConfig.RightKey) & 0x8000) currentInput |= FLAG_RIGHT;
	if (GetAsyncKeyState(KeyConfig.UpKey) & 0x8000) currentInput |= FLAG_UP;
	if (GetAsyncKeyState(KeyConfig.DownKey) & 0x8000) currentInput |= FLAG_DOWN;

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

void UPikachu::SetKeyConfig(const FPlayerKeyConfig& config)
{
	KeyConfig = config;
}