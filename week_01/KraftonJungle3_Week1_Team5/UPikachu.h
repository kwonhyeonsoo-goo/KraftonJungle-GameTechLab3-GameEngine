#pragma once
#include <string>

#include "fmod_common.h"
#include "UGameObject.h"
class USphereMesh;
class UShader;
class UCubeMesh;
class UBall;
class TextureRenderer;
class Animator;

enum EInputFlag
{
	FLAG_NONE = 0,
	FLAG_SPIKE = 1 << 0,  // 1  (0000 0001)
	FLAG_LEFT = 1 << 1,  // 2  (0000 0010)
	FLAG_RIGHT = 1 << 2,  // 4  (0000 0100)
	FLAG_UP = 1 << 3,  // 8  (0000 1000)
	FLAG_DOWN = 1 << 4   // 16 (0001 0000)
};

enum class EPlayerState
{
	Normal,				// 기본 (이동, 대기)
	Diving,
	Recovering,
	BasicSpike,			// 기본 스파이크
	FrontSpike,			// 좌우 방향 스파이크
	UpSpike,			// 위 스파이크
	DownSpike,			// 아래 스파이크
	UpFrontSpike,
	DownFrontSpike,
	Win,
	Lose
};

struct FPlayerKeyConfig
{
	int UpKey;
	int DownKey;
	int LeftKey;
	int RightKey;
	int SpikeKey;
};

class UPikachu : public UGameObject
{
public:
	EPlayerState GetPlayerState() const { return CurrentState; }
	void SetKeyConfig(const FPlayerKeyConfig& config);
	void SetBoundary(float left, float right, float top, float bottom);

	void Create(ID3D11Device* device, ID3D11DeviceContext* context);
	void Release() override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;

	void HandleCollision(UBall* ball);

	void Physics_Update(float tick) override;
	void Update(float tick) override;

	EPlayerState GetSpikeStateFromInput(int input);

	bool GetIsAI() const { return bIsAI; }
	void SetIsAI(bool isAI) { bIsAI = isAI; }

	void SetTargetBall(UBall* ball) { TargetBall = ball; }
	void SetMyFinalSCore(int FinalScore);
#undef PlaySound
	void PlaySound(const std::wstring& sound) const;

	void SetType(const EPlayerType type) { Type = type; }

private:
	void ApplyBoundaryCollision();
	void Move(float tick);
	float PredictLandingX(float startX, float startY, float velX, float velY, float ballRadius);

private:
	//UCubeMesh* CubeMesh;
	//USphereMesh* SphereMesh;
	//UShader* Shader;

	EPlayerState CurrentState = EPlayerState::Normal;
	FPlayerKeyConfig KeyConfig;
	float JumpForce;
	bool bOnGround;
	float RecoveryTimer;

	bool bIsAI = false;
	UBall* TargetBall = nullptr;

	float LeftBorder = -1.0f;
	float RightBorder = 1.0f;
	float TopBorder = 1.0f;
	float BottomBorder = -1.0f;

	EPlayerType Type = EPlayerType::None;


public:
	Animator* AnimatorComponent;

	int MyFinalScore;
	bool DefalutFlip;
	TextureRenderer* TextureRender;
};