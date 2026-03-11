#pragma once

enum class ColliderType
{
	ColliderType_Circle,
	ColliderType_Rect
};

enum class ObjectType
{
	None,
	Ball,
	Pikachu,
	screen,

	COUNT
};

enum class EGameState
{
	None,				// 초기 상태
	Idle,				// Init 단계
	Serving,			// 서브 시작 단계
	Playing,			// 게임 플레이 단계
	PointScored,		// 점수 획득 단계
	SetEnd,				// 점수 판별 및 세트 종료 단계
	GameOver,			// 누군가의 Max 점 득점으로 게임 종료

	COUNT
};

enum class EServerOwner
{
	Player1, 
	Player2, 
	
	COUNT
};

enum CHANNELID
{
	SOUND_EFFECT,
	SOUND_PLAYER_1,
	SOUND_PLAYER_1_EFFECT,
	SOUND_PLAYER_2,
	SOUND_PLAYER_2_EFFECT,
	SOUND_BGM,
	SOUND_UI,

	MAXCHANNEL
};

enum class EPlayerType
{
	Player1,
	Player2,
	None
};