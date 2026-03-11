#pragma once
#include "UScene.h"

class UWeek0Scene : public UScene
{
public:
	void Enter() override;
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Update(float tick) override;
	void Exit() override;
};

