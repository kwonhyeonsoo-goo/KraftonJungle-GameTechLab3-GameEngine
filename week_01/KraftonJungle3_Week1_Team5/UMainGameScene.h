#pragma once
#include "UScene.h"

class UUIImage;
class UNet;

class UMainGameScene : public UScene
{
public:
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Update(float tick) override;
	void Exit() override;

	void CheckCollision();

	void InitializeUI(ID3D11Device* device, ID3D11DeviceContext* context);

	void UpdateCloudImageAnimation(float tick);

private:
	std::vector<float> CloudAnimationTime;
	std::vector<UUIImage*> Clouds;
	UNet* Net;

	static constexpr float CloudBaseScale = 0.75f;
	static constexpr float CloudScaleAmplitude = 1.f;
	static constexpr float CloudPulseSpeed = 10.f;
	static constexpr int CloudCount = 10;
};

