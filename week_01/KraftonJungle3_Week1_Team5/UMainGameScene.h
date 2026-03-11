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

private:
	UNet* Net;
	UPikachu* Player1;
	UPikachu* Player2;
};

