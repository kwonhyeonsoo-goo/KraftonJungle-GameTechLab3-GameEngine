#pragma once
#include "UGameObject.h"
class USphereMesh;
class UShader;
class UCubeMesh;

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
	DownFrontSpike
};

class UPikachu : public UGameObject
{
public:
	EPlayerState GetPlayerState() const { return CurrentState; }

	void Create(ID3D11Device* device, ID3D11DeviceContext* context);
	void Release() override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;

	void Physics_Update(float tick) override;
	void Update(float tick) override;

private:
	void ApplyBoundaryCollision();
	void Move(float tick);

	EPlayerState GetSpikeStateFromInput(int input);

private:
	UCubeMesh* CubeMesh;
	USphereMesh* SphereMesh;
	UShader* Shader;

	EPlayerState CurrentState = EPlayerState::Normal;
	float JumpForce;
	bool bOnGround;
	float RecoveryTimer;

	const float LeftBorder = -1.0f;
	const float RightBorder = 1.0f;
	const float TopBorder = 1.0f;
	const float BottomBorder = -1.0f;
};