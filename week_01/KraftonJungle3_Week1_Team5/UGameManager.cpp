#include "UGameManager.h"
#include "UBall.h"
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
		float StateTimer = 1.5f;
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
	bool bResult = CheckBallFloor();
	if (bResult)
	{
		GameState = EGameState::PointScored;

		float xPos = PocketBall->GetPosition().x;

		if (xPos >= -1.f && xPos <= -0.02f) // Player1 구역에서 true 라면
		{
			ScorePoint(1); // Player 2 점수 증가
			ServerOwner = EServerOwner::Player2;
		}
		else
		{
			ScorePoint(0); // Player 1 점수 증가
			ServerOwner = EServerOwner::Player1;
		}
		if (GameState != EGameState::GameOver)
		{
			GameState = EGameState::SetEnd;
			ResetRound(); //라운드 초기화
		}
	}
}

bool UGameManager::CheckBallFloor()
{
	float Radius = PocketBall->GetRadius();
	float yPos = PocketBall->GetPosition().y;
	float diff = ( yPos) - (-1.f + Radius);
	bool bResult = (diff < 0.00000000001);

	return bResult;
}

void UGameManager::ScorePoint(int player)
{
	player == 0 ? P1Point++ : P2Point++;

	if (P1Point >= MaxPoint || P2Point >= MaxPoint)
	{
		GameState = EGameState::GameOver;
		//TODO: 엔터키 입력 전까지 해당 화면을 유지?
	}

}

void UGameManager::ResetRound()
{
	GameState = EGameState::Serving;

	// Player 위치 초기화
	Player1->SetPosition(Player1Pos);
	Player2->SetPosition(Player2Pos);

	if (ServerOwner == EServerOwner::Player2)
	{
		PocketBall->SetPosition(FVector3(Player2Pos.x, .7f, 0.f));
		PocketBall->SetVelocity(FVector3(0.f, -.5f, 0.f));
	}
	else
	{
		PocketBall->SetPosition(FVector3(Player1Pos.x, .7f, 0.f));
		PocketBall->SetVelocity(FVector3(0.f, -.5f, 0.f));
	}

	GameState = EGameState::Playing;

}

