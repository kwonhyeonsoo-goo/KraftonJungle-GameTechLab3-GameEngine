#pragma once
#include <string>
#include <vector>
#include <map>
#include <d3d11.h>
#include "UEngine.h"
#include "TextureRenderer.h"

class UTexture2D;

class Animator
{
public:
	Animator() ;
	~Animator();

	void SetFrameDuration(float duration) { FrameDuration = duration; }

	void Play(TextureRenderer* renderer, const std::string& state, float tick);
	void AddFrames(const std::string& state, const std::vector<std::wstring>& filePaths);
	void Update(float tick);
	UTexture2D* GetCurrentFrame() const { return _currentTexture; }

private:
	std::map<std::string, std::vector<UTexture2D*>> Animations;
	float FrameDuration = 0.1f; // 각 프레임이 지속되는 시간 (초)
	float ElapsedTime = 0.0f; // 현재 프레임이 얼마나 지속되었는지 추적
	std::wstring CurrentframePath; // 현재 프레임의 파일 경로
	UTexture2D* _currentTexture; // 현재 프레임의 텍스처 포인터
};

