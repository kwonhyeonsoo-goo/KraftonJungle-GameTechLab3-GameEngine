#include "UPikachu.h"

#include "UCubeMesh.h"
#include "USphereMesh.h"
#include "UShader.h"
#include "Utility.h"
#include "Enum.h"
#include "UBall.h"
#include <algorithm>
#include "TextureRenderer.h"
#include "Animator.h"
#include "UGameManager.h"
#include "UEngine.h"

void UPikachu::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	SetObjectType(ObjectType::Pikachu);

	/*CubeMesh = new UCubeMesh();
	CubeMesh->CreateCube(device);*/
	//SphereMesh = new USphereMesh();
	//SphereMesh->CreateSphere(device);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "Color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
			D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	//Shader = new UShader();
	//Shader->Create(device, L"ShaderW0.hlsl", layout, ARRAYSIZE(layout), "mainVS", "mainPS");

	UseGravity = true;
	JumpForce = 3.0f;
	bOnGround = false;
	RecoveryTimer = 0.25f;
	MyFinalScore = 0;

#pragma region Texture&Animation
	TextureRender = new TextureRenderer();
	TextureRender->Create(device, context);
	TextureRender->Init(device, context, L"sprite_sheet.png");


	AnimatorComponent = new Animator();
	AnimatorComponent->SetFrameDuration(0.1f);

	std::vector<std::wstring> idleFrames = {
	L"Resources/Textures/pikachu/pikachu_0_0.png",
	L"Resources/Textures/pikachu/pikachu_0_1.png",
	L"Resources/Textures/pikachu/pikachu_0_2.png",
	L"Resources/Textures/pikachu/pikachu_0_3.png",
	L"Resources/Textures/pikachu/pikachu_0_4.png"
	};
	AnimatorComponent->AddFrames("Normal", idleFrames);


	std::vector<std::wstring> jumpFrames = {
	L"Resources/Textures/pikachu/pikachu_1_0.png",
	L"Resources/Textures/pikachu/pikachu_1_1.png",
	L"Resources/Textures/pikachu/pikachu_1_2.png",
	L"Resources/Textures/pikachu/pikachu_1_3.png",
	L"Resources/Textures/pikachu/pikachu_1_4.png"
	};
	AnimatorComponent->AddFrames("Jump", jumpFrames);


	std::vector<std::wstring> Spike1Frames = {
	L"Resources/Textures/pikachu/pikachu_2_0.png",
	L"Resources/Textures/pikachu/pikachu_2_1.png",
	L"Resources/Textures/pikachu/pikachu_2_2.png",
	L"Resources/Textures/pikachu/pikachu_2_3.png",
	L"Resources/Textures/pikachu/pikachu_2_4.png"
	};
	AnimatorComponent->AddFrames("Spike", Spike1Frames);


	std::vector<std::wstring> Spike2Frames = {

	};
	AnimatorComponent->AddFrames("Spike2", Spike2Frames);


	std::vector<std::wstring> DiveFrames = {
L"Resources/Textures/pikachu/pikachu_3_0.png",
L"Resources/Textures/pikachu/pikachu_3_1.png",
L"Resources/Textures/pikachu/pikachu_3_2.png",
	};
	AnimatorComponent->AddFrames("Diving", DiveFrames);


	std::vector<std::wstring> RecoverFrames = {
	L"Resources/Textures/pikachu/pikachu_4_0.png",
	};
	AnimatorComponent->AddFrames("Recovering", RecoverFrames);


	std::vector<std::wstring> WinFrames = {
L"Resources/Textures/pikachu/pikachu_5_0.png",
L"Resources/Textures/pikachu/pikachu_5_1.png",
L"Resources/Textures/pikachu/pikachu_5_2.png",
L"Resources/Textures/pikachu/pikachu_5_3.png",
L"Resources/Textures/pikachu/pikachu_5_4.png"
	};
	AnimatorComponent->AddFrames("Win", WinFrames);


	std::vector<std::wstring> LoseFrames = {
L"Resources/Textures/pikachu/pikachu_6_0.png",
L"Resources/Textures/pikachu/pikachu_6_1.png",
L"Resources/Textures/pikachu/pikachu_6_2.png",
L"Resources/Textures/pikachu/pikachu_6_3.png",
L"Resources/Textures/pikachu/pikachu_6_4.png"
	};
	AnimatorComponent->AddFrames("Lose", LoseFrames);

	AnimatorComponent->Play("Win", AnimationMode::Once);

#pragma endregion
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
	//게임오버시
	if (UGameManager::GetInstance().GetGameState() == EGameState::GameOver)
	{
		if (MyFinalScore == UGameManager::GetInstance().GetMaxPoint())
		{
			AnimatorComponent->Play("Win", AnimationMode::Once);
		}
		else
			AnimatorComponent->Play("Lose", AnimationMode::Once);
	}
	else
	{
		if (!bOnGround) {
			if (GetPlayerState() == EPlayerState::BasicSpike || GetPlayerState() == EPlayerState::FrontSpike ||
				GetPlayerState() == EPlayerState::UpSpike || GetPlayerState() == EPlayerState::DownSpike ||
				GetPlayerState() == EPlayerState::UpFrontSpike || GetPlayerState() == EPlayerState::DownFrontSpike
				)
			{
				AnimatorComponent->Play("Spike", AnimationMode::Loop);
				PlaySound(L"Pikachu_Smash.wav");
			}
			else if (GetPlayerState() == EPlayerState::Diving) {
				AnimatorComponent->Play("Diving", AnimationMode::Loop);
			}
			else {
				AnimatorComponent->Play("Jump", AnimationMode::Round);
			}
		}
		else if (GetPlayerState() == EPlayerState::Normal) {
			TextureRender->SetFlipDraw(DefalutFlip);
			AnimatorComponent->Play("Normal", AnimationMode::Round);

		}
		else if (GetPlayerState() == EPlayerState::Recovering) {
			AnimatorComponent->Play("Recovering", AnimationMode::Loop);

		}
	}

	AnimatorComponent->Update(TextureRender, tick);
}


void UPikachu::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	//SphereMesh->Bind(context);
	//Shader->Bind(context);
	//Shader->UpdateConstant(context, Position, Scale);
	//SphereMesh->Draw(context);

	TextureRender->Draw(context, device, Position, 1.f);

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
		BallPos = ball->GetPosition();
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
	float newYVel = fabsf(ball->GetVelocity().y) + fabsf(Velocity.y);
	if (newYVel < 1.5f) newYVel = 1.5f;
	if (BallPos.y < Position.y) newYVel *= -1;
	

	// 플레이어가 왼쪽, 오른쪽인지에 따라 방향 구분
	float xSign = (Position.x <= -0.02f && Position.x >= -1.0f) ? 1.f : -1.f;

	switch (CurrentState)
	{
	case EPlayerState::BasicSpike:
		// 앞으로 적당하게
		newXVel = xSign * 2.5f;
		newYVel = 0.0f;
		ball->SetSpike(true, Position);
		break;

	case EPlayerState::FrontSpike:
		// 옆
		newXVel = xSign * 3.0f;
		newYVel = 0.0f;
		ball->SetSpike(true, Position);

		break;

	case EPlayerState::UpSpike:
		// 위
		newXVel = xSign;
		newYVel = 3.0f;
		ball->SetSpike(true, Position);

		break;

	case EPlayerState::DownSpike:
		// 아
		newXVel = xSign;
		newYVel = -3.0f;
		ball->SetSpike(true, Position);

		break;

	case EPlayerState::UpFrontSpike:
		// 위 + 앞 대각선
		newXVel = xSign * 2.0f;
		newYVel = 3.0f;
		ball->SetSpike(true, Position);

		break;

	case EPlayerState::DownFrontSpike:
		// 앞 + 아래 대각선
		newXVel = xSign * 2.0f;
		newYVel = -3.0f;
		ball->SetSpike(true, Position);
		break;

	default: // Normal
		newYVel = min(newYVel, 2.5f);
		ball->SetSpike(false);

		break;
	}

	// 속력 제한
	const float MaxBallSpeed = 3.0f; // 스파이크(강공) 속도
	const float MinBallSpeed = 2.5f; // 일반 공격 속도
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
	if (TextureRender)
	{
		delete TextureRender;
		TextureRender = nullptr;
	}

	if (AnimatorComponent)
	{
		delete AnimatorComponent;
		AnimatorComponent = nullptr;
	}

}

void UPikachu::SetBoundary(float left, float right, float top, float bottom)
{
	LeftBorder = left;
	RightBorder = right;
	TopBorder = top;
	BottomBorder = bottom;
}

void UPikachu::SetMyFinalSCore(int FinalScore)
{
	MyFinalScore = FinalScore;
	if (MyFinalScore == UGameManager::GetInstance().GetMaxPoint())
	{
		PlaySound(L"Pikachu_Win.wav");
	}
}

void UPikachu::PlaySound(const std::wstring& sound) const
{
	if (Type == EPlayerType::Player1)
	{
		UEngine::GetInstance().GetSoundManager().PlaySound(sound, SOUND_PLAYER_1, 0.8f);
	}

	if (Type == EPlayerType::Player2)
	{
		UEngine::GetInstance().GetSoundManager().PlaySound(sound, SOUND_PLAYER_2, 0.8f);
	}
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
			TextureRender->SetFlipDraw(DefalutFlip);
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

	if (!bIsAI)
	{
		// 사람의 키보드 조작
		if (GetAsyncKeyState(KeyConfig.SpikeKey) & 0x8000) currentInput |= FLAG_SPIKE;
		if (GetAsyncKeyState(KeyConfig.LeftKey) & 0x8000) currentInput |= FLAG_LEFT;
		if (GetAsyncKeyState(KeyConfig.RightKey) & 0x8000) currentInput |= FLAG_RIGHT;
		if (GetAsyncKeyState(KeyConfig.UpKey) & 0x8000) currentInput |= FLAG_UP;
		if (GetAsyncKeyState(KeyConfig.DownKey) & 0x8000) currentInput |= FLAG_DOWN;
	}
	else
	{
		if (TargetBall)
		{
			FVector3 ballPos = TargetBall->GetPosition();
			FVector3 ballVel = TargetBall->GetVelocity();

			float PositionLeftBorder = -1.0f + TargetBall->GetRadius();
			float PositionRightBorder = 1.0f - TargetBall->GetRadius();

			// 공의 다음 X 위치 예측
			float targetX = ballPos.x + (ballVel.x * tick * 10.0f);
			float targetY = ballPos.y + (ballVel.y * tick * 10.0f);

			for (int i = 0; i < 3; ++i)
			{
				if (PositionLeftBorder <= targetX && targetX <= PositionRightBorder)
				{
					break;
				}

				if (targetX < PositionLeftBorder)
				{
					targetX = PositionLeftBorder + (PositionLeftBorder - targetX) * 0.9f;
				}
				else if (targetX > PositionRightBorder)
				{
					targetX = PositionRightBorder - (targetX - PositionRightBorder) * 0.9f;
				}
			}

			float myTargetX = targetX;
			if (myTargetX < LeftBorder) myTargetX = LeftBorder;
			if (myTargetX > RightBorder) myTargetX = RightBorder;

			float deadzone = 0.05f;

			// 이동
			if (Position.x < myTargetX - deadzone)
			{
				currentInput |= FLAG_RIGHT;
			}
			else if (Position.x > myTargetX + deadzone)
			{
				currentInput |= FLAG_LEFT;
			}

			// 1P 2P 판별
			bool bIsPlayer1 = (LeftBorder < 0.0f && RightBorder <= 0.0f);

			// 점프
			if (bOnGround && -0.15f < targetY && targetY < 0.2f && abs(Position.x - targetX) < 0.2f && ballPos.y - Position.y > 0.7f)
			{
				currentInput |= FLAG_UP;
			}

			// 다이빙
			if (bOnGround && targetY < -0.3f && abs(Position.x - myTargetX) > 0.2f)
			{
				currentInput |= FLAG_SPIKE;
			}

			// 스파이크
			if (!bOnGround && ballPos.y > -0.1f && abs(Position.x - ballPos.x) < 0.2f)
			{
				currentInput |= FLAG_SPIKE;
				currentInput |= (rand() % 16) << 1;
			}
		}
	}

	// 2. 이동
	if (currentInput & FLAG_LEFT) Position.x -= 1.0f * tick;
	if (currentInput & FLAG_RIGHT) Position.x += 1.0f * tick;

	if (bOnGround)
	{
		if (currentInput & FLAG_UP)
		{
			PlaySound(L"Pikachu_Jump.wav");
			Velocity.y = JumpForce;
			bOnGround = false;
		}
		else if (currentInput & FLAG_SPIKE)
		{
			if (currentInput & FLAG_LEFT)
			{
				CurrentState = EPlayerState::Diving;
				PlaySound(L"Pikachu_Jump.wav");
				Velocity.x = -1.2f;
				Velocity.y = JumpForce * 0.3f;
				bOnGround = false;
			}
			else if (currentInput & FLAG_RIGHT)
			{
				CurrentState = EPlayerState::Diving;
				TextureRender->SetFlipDraw(!DefalutFlip);

				PlaySound(L"Pikachu_Jump.wav");
				Velocity.x = 1.2f;
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