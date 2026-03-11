#pragma once
#include "UScene.h"

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

	// 임시
	void OnImGuiRender() override;

private:
	UPikachu* Player1 = nullptr;
	UPikachu* Player2 = nullptr;

	UNet* Net;
};

