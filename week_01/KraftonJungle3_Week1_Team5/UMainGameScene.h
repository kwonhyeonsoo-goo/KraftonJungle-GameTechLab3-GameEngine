#pragma once
#include "UScene.h"

class UUIImage;
class UNet;
class UPikachu;

class UMainGameScene : public UScene
{
public:
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Update(float tick) override;
	void Exit() override;

	void CheckCollision();

	void InitializeUI(ID3D11Device* device, ID3D11DeviceContext* context);

	void UpdateCloudImageAnimation(float tick);
	void UpdateCloudMovement(UUIImage* cloud);

private:
	std::vector<float> CloudAnimationTime;
	std::vector<UUIImage*> Clouds;
	UNet* Net;
	UPikachu* Player1;
	UPikachu* Player2;

	static constexpr float CloudBaseScale = 0.9f;
	static constexpr float CloudScaleAmplitude = 0.3f;
	static constexpr float CloudPulseSpeed = 10.f;
	static constexpr int CloudCount = 20;
};

