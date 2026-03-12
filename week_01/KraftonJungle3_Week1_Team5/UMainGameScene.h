#pragma once
#include "UScene.h"

class UUIScore;
class UUIImage;
class UNet;
class UPikachu;

class UMainGameScene : public UScene
{
public:
	void Enter() override;
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Update(float tick) override;
	void Exit() override;

	void CheckCollision();

	void InitializeUI(ID3D11Device* device, ID3D11DeviceContext* context);

	// 임시
	void OnImGuiRender() override;

	void UpdateCloudImageAnimation(float tick);
	void UpdateCloudMovement(UUIImage* cloud);

private:
	std::vector<float> CloudAnimationTime;
	std::vector<UUIImage*> Clouds;

	UPikachu* Player1 = nullptr;
	UPikachu* Player2 = nullptr;
	UNet* Net = nullptr;

	UUIScore* P1_Score = nullptr;
	UUIScore* P2_Score = nullptr;

	UUIImage* GameSetUI = nullptr;
	float GameSetUIAnimationDurationTime = 0.f;

	static constexpr float CloudBaseScale = 0.9f;
	static constexpr float CloudScaleAmplitude = 0.3f;
	static constexpr float CloudPulseSpeed = 10.f;
	static constexpr int CloudCount = 20;
	
	static constexpr float GameSetBaseScale = 10.f;
	static constexpr float GameSetTargetScale = 1.f;
	static constexpr float GameSetLerpTime = 1.5f;
};

