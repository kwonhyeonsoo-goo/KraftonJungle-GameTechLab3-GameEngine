#pragma once
#include <array>
#include <vector>

#include "FVector3.h"
#include "UScene.h"

class UUIButton;
class UUIImage;

class UMainTitleScene : public UScene
{
public:
	void Enter() override;
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Update(float tick) override;
	void Exit() override;

private:
	bool InitializeBackgroundTiles(ID3D11Device* device, ID3D11DeviceContext* context);
	void UpdateBackgroundAnimation(float tick);
	void UpdateBackgroundTilePositions() const;
	void UpdateMenuVisuals() const;
	void UpdateFightImageAnimation(float tick);
	bool IsKeyJustPressed(int virtualKey, bool& bWasPressed) const;
	void ExecuteSelectedMenu() const;

private:
	std::vector<UUIImage*> BackgroundTiles;
	std::array<UUIButton*, 3> MenuButtons{ nullptr, nullptr, nullptr };
	std::array<FVector3, 3> MenuButtonPositions{};
	UUIImage* FightImage = nullptr;
	FVector3 BackgroundTileStep = FVector3{ 0.0f, 0.0f, 0.0f };
	FVector3 BackgroundScrollOffset = FVector3{ 0.0f, 0.0f, 0.0f };
	int BackgroundTileColumns = 0;
	int BackgroundTileRows = 0;
	float FightAnimationTime = 0.0f;
	int SelectedMenuIndex = 0;
	bool bWasUpPressed = false;
	bool bWasDownPressed = false;
	bool bWasEnterPressed = false;

	static constexpr float BackgroundTileScale = 1.0f;
	static constexpr float BackgroundScrollSpeed = 0.12f;
	static constexpr float FightBaseScale = .2f;
	static constexpr float FightScaleAmplitude = 1.0f;
	static constexpr float FightPulseSpeed = 10.0f;
};
