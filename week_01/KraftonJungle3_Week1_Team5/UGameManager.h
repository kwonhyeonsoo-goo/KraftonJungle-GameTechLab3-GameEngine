#pragma once

#include "UPrimitive.h"
#include "Macro.h"
#include "Enum.h"
#include "FVector3.h"

class UPikachu;
class UBall;

class UGameManager : public UPrimitive
{
	UGameManager();
	~UGameManager() override;

	DECLARE_SINGLETON(UGameManager)

	void Initialize(UPikachu* player1, UPikachu* player2, UBall* ball,
		FVector3 player1Pos, FVector3 player2Pos);						//게임 매니저 초기화 하는 함수
	void Update(float deltaTime);
	void CheckScore();
	void ScorePoint(int player);										// 점수 처리
	void ResetRound();													// 위치 초기화

	EGameState GetGameState() const { return GameState; }
	int GetP1Point() const { return P1Point; }
	int GetP2Point() const { return P2Point; }
	int GetMaxPoint() const { return MaxPoint; }

	void SetAIMode(bool active) { bAIMode = active; }
	bool IsAIMode() const { return bAIMode; }

	void SetAIVsAIMode(bool active) { bAIVsAIMode = active; }
	bool IsAIVsAIMode() const { return bAIVsAIMode; }

private:
	EGameState GameState; //현재 게임 흐름
	EServerOwner ServerOwner; //서브 주도권 쥐고 있는 플레이어
	float StateTimer; //PointScored->Serving 까지 타이머
	float MaxStateTimer;

	int P1Point; //P1 점수
	int P2Point; //P2 점수

	int MaxPoint;

	//플레이어 1, 2, 공 캐싱
	UPikachu* Player1 = nullptr;
	UPikachu* Player2 = nullptr;
	UBall* PocketBall = nullptr;

	FVector3 Player1Pos, Player2Pos; // 게임 시작시 플레이어 위치

	bool bAIMode = false;
	bool bAIVsAIMode = false;

};

