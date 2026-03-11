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
	Idle,
	Serving,
	Playing,
	PointScored,
	SetEnd,
	GameOver,

	COUNT
};

enum class EServerOwner
{
	Player1, 
	Player2, 
	
	COUNT
};