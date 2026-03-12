#include "UGameManager.h"
#include "UBall.h"
#include "UEngine.h"
#include "UPikachu.h"

IMPLEMENT_SINGLETON(UGameManager)

UGameManager::UGameManager()
{

}

UGameManager::~UGameManager()
{

}

void UGameManager::Initialize(UPikachu* player1, UPikachu* player2, UBall* ball,
	FVector3 player1Pos, FVector3 player2Pos)
{
	{
		GameState = EGameState::Serving;
		ServerOwner = EServerOwner::Player1;
		MaxStateTimer = 1.f;
		StateTimer = MaxStateTimer;
		P1Point = 0;
		P2Point = 0;

		Player1 = player1;
		Player2 = player2;
		PocketBall = ball;

		Player1Pos = player1Pos;
		Player2Pos = player2Pos;

		MaxPoint = 5;
	}
	
	ResetRound();
}

void UGameManager::Update(float deltaTime)
{
	if (GameState == EGameState::GameOver)
		return;

	if (GameState == EGameState::PointScored)
	{
		StateTimer -= deltaTime;
		if (StateTimer <= 0.f)
		{
			StateTimer = MaxStateTimer; // 타이머 초기화
			ResetRound();              // 시간이 다 됐을 때 리셋
		}
		return;
	}

	if (GameState == EGameState::Serving)
	{
		StateTimer -= deltaTime;
		if (StateTimer <= 0.f)
		{
			GameState = EGameState::Playing;
		}
		return;
	}
}

void UGameManager::CheckScore()
{
	if (GameState == EGameState::Playing)
	{
		float xPos = PocketBall->GetPosition().x;
		if (xPos >= -1.f && xPos <= -0.02f)
		{
			ScorePoint(1);
			ServerOwner = EServerOwner::Player2;
		}
		else
		{
			ScorePoint(0);
			ServerOwner = EServerOwner::Player1;
		}

		// GameOver가 아니면 타이머 시작
		if (GameState != EGameState::GameOver)
		{
			GameState = EGameState::PointScored;
			StateTimer = MaxStateTimer; // 타이머 시작
		}
	}
}

void UGameManager::ScorePoint(int player)
{
	player == 0 ? P1Point++ : P2Point++;
	UEngine::GetInstance().GetSoundManager().PlaySound(L"Ball_GroundHit.wav", SOUND_EFFECT, SYSTEM_VOLUME);
	if (P1Point >= MaxPoint || P2Point >= MaxPoint)
	{
		GameState = EGameState::GameOver;
		Player1->SetMyFinalSCore(P1Point);
		FVector3 P1Pos = Player1->GetPosition();
		Player1->SetPosition(FVector3(P1Pos.x, -0.65f, 0.f));

		Player2->SetMyFinalSCore(P2Point);
		FVector3 P2Pos = Player2->GetPosition();
		Player2->SetPosition(FVector3(P2Pos.x, -0.65f, 0.f));
	}
}

void UGameManager::ResetRound()
{
	// Player 위치 초기화
	Player1->SetPosition(FVector3(Player1Pos.x, Player1Pos.y + 0.1f, Player1Pos.z));
	Player2->SetPosition(FVector3(Player2Pos.x, Player2Pos.y + 0.1f, Player2Pos.z));

	//서브시 각 서브권마다 공이 시작되는 곳
	if (ServerOwner == EServerOwner::Player2)
	{
		PocketBall->SetPosition(FVector3(Player2Pos.x, .7f, 0.f));
		PocketBall->SetVelocity(FVector3(0.f, 0.0f, 0.f));
	}
	else
	{
		PocketBall->SetPosition(FVector3(Player1Pos.x, .7f, 0.f));
		PocketBall->SetVelocity(FVector3(0.f, 0.0f, 0.f));
	}
	GameState = EGameState::Playing;
	PocketBall->Init();
}

