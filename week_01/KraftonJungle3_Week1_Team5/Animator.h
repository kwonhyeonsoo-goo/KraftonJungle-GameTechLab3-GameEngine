#pragma once
#include <string>
#include <vector>
#include <map>

class TextureRenderer;
class UTexture2D;

enum class AnimationMode
{
	Loop,
	Once,
	Round
};


class Animator
{
public:
	Animator() ;
	~Animator();

	void SetFrameDuration(float duration) { FrameDuration = duration; }
	void Initialize() { ElapsedTime = 0.0f; }

	void Play(std::string state, AnimationMode mode);
	void Update(TextureRenderer* renderer, float tick);

	void PlayLoop(TextureRenderer* renderer, const std::string& state, float tick);
	bool PlayOnce(TextureRenderer* renderer, const std::string& state, float tick);	//fasle면 애니메이션 재생 중, true면 애니메이션 끝남
	void PlayRound(TextureRenderer* renderer, const std::string& state, float tick);

	void AddFrames(const std::string& state, const std::vector<std::wstring>& filePaths);
	UTexture2D* GetCurrentFrame() const { return _currentTexture; }
	std::string GetCurrentState() const { return CurrentState; }
	bool isPlaying() const { return isPlayingAnimation; }	

private:
	std::map<std::string, std::vector<UTexture2D*>> Animations;
	float FrameDuration = 0.1f; // 각 프레임이 지속되는 시간 (초)
	float ElapsedTime = 0.0f; // 현재 프레임이 얼마나 지속되었는지 추적
	std::wstring CurrentframePath; // 현재 프레임의 파일 경로
	UTexture2D* _currentTexture; // 현재 프레임의 텍스처 포인터

	AnimationMode CurrentMode; // 현재 애니메이션 모드
	std::string CurrentState; // 현재 애니메이션 상태

	bool isPlayingAnimation = false; // 애니메이션 재생 여부
};

