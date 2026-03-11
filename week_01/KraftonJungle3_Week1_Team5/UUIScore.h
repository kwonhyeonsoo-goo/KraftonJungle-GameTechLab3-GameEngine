#pragma once
#include <vector>

#include "UUIImage.h"
class UTexture2D;

class UUIScore : public UUIImage
{
public:
	UUIScore() = default;
	~UUIScore() override = default;

	void SetScore(int score) { Score = score; }

	void OnCreate(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void OnUpdate(float tick) override;

private:
	std::vector<UTexture2D*> Numbers;
	int Score = 0;
};

