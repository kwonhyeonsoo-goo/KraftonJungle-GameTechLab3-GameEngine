#pragma once
#include <array>

#include "FVector3.h"
#include "UScene.h"

class UUIButton;
class UUIImage;

class UMainTitleScene : public UScene
{
public:
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Update(float tick) override;
	void Exit() override;

private:
	void UpdateMenuVisuals() const;
	void UpdateFightImageAnimation(float tick);
	bool IsKeyJustPressed(int virtualKey, bool& bWasPressed) const;
	void ExecuteSelectedMenu() const;

private:
	std::array<UUIButton*, 2> MenuButtons{ nullptr, nullptr };
	std::array<FVector3, 2> MenuButtonPositions{};
	UUIImage* FightImage = nullptr;
	UUIImage* SelectionMark = nullptr;
	float FightAnimationTime = 0.0f;
	int SelectedMenuIndex = 0;
	bool bWasUpPressed = false;
	bool bWasDownPressed = false;
	bool bWasEnterPressed = false;

	static constexpr float FightBaseScale = .8f;
	static constexpr float FightScaleAmplitude = 2.0f;
	static constexpr float FightPulseSpeed = 10.0f;
};
