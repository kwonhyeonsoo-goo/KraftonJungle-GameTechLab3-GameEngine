#pragma once
#include "UGameObject.h"

class UUIImage;

class UWave :
	public UGameObject
{
public:
	UWave();
	~UWave() override = default;

	void Physics_Update(float tick) override;
	void Update(float tick) override; // tick = delta time 이라고 보면 됨.
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;
	const char* GetEditorTypeName() const { return "UWave"; }

	void Create(ID3D11Device* device, ID3D11DeviceContext* context);
	void Release() override;
private:
	UUIImage* Waves[27];
	float velocity=0.5f;
	int timeAccumulator = 0;
	int prevFrameStep = -1;
	float FrameDuration = 0.04f; // 애니메이션 주기 (초)
	float ElaspedTime = 0.f; // 애니메이션 타이머

};

