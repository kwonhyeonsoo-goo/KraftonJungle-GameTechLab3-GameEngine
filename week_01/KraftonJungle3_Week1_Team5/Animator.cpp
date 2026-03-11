#include "Animator.h"

#include "TextureRenderer.h"
#include "UEngine.h"

Animator::Animator()
{
	ElapsedTime = 0.0f;
	FrameDuration = 0.1f;
	_currentTexture = nullptr;
}

// 2. 소멸자 몸통
Animator::~Animator()
{
	// 나중에 메모리 해제할 게 생기면 여기에 작성
}
void Animator::Play(std::string state, AnimationMode mode)
{
	if (mode != CurrentMode || state != CurrentState){
            ElapsedTime = 0.0f; // 애니메이션 시작 시 시간 초기화
            CurrentMode = mode;
            CurrentState = state;
            isPlayingAnimation = true;
	}
}

void Animator::Update(TextureRenderer* renderer, float tick)
{
    std::vector<UTexture2D*>& frames = Animations[CurrentState];

    if (Animations.find(CurrentState) == Animations.end() || Animations[CurrentState].empty())
        return;
    if (FrameDuration <= 0.0f) return;


    int frameIndex;
    ElapsedTime += tick;


    if (CurrentMode == AnimationMode::Loop)
    {

        frameIndex = static_cast<int>(ElapsedTime / FrameDuration) % frames.size();

		isPlayingAnimation = true;

    }
    else if (CurrentMode == AnimationMode::Once)
    {

        frameIndex = static_cast<int>(ElapsedTime / FrameDuration) % frames.size();
        bool isAnimationFinished = (ElapsedTime / FrameDuration) >= frames.size();

        if (!isAnimationFinished) {
			isPlayingAnimation = true;
        }
        else
        {
            frameIndex = frames.size() - 1;
			isPlayingAnimation = false;
        }

    }
    else if (CurrentMode == AnimationMode::Round)
    {

        int frameCount = frames.size();
        isPlayingAnimation = true;
        if (frameCount == 1)
        {
            frameIndex = 0;
        }
        else {
            int cycle = frameCount * 2 - 2;

            int index = static_cast<int>(ElapsedTime / FrameDuration) % cycle;


            if (index < frameCount)
                frameIndex = index;
            else
                frameIndex = cycle - index;
        }
    }

    _currentTexture = frames[frameIndex];
    renderer->SetTexture(_currentTexture);
}

void Animator::PlayLoop(TextureRenderer* renderer, const std::string& state, float tick)
{

    if (Animations.find(state) == Animations.end() || Animations[state].empty())
        return;

    std::vector<UTexture2D*>& frames = Animations[state];

    if (FrameDuration <= 0.0f) return;

    // 2. 시간 누적 및 인덱스 계산
    ElapsedTime += tick;
    int frameIndex = static_cast<int>(ElapsedTime / FrameDuration) % frames.size();

    // 3. Renderer에게 텍스처 포인터를 직접 전달 (매우 빠름)
    _currentTexture = frames[frameIndex];
    renderer->SetTexture(_currentTexture);
	
}

bool Animator::PlayOnce(TextureRenderer* renderer, const std::string& state, float tick)
{

    if (Animations.find(state) == Animations.end() || Animations[state].empty())
        return true;

    std::vector<UTexture2D*>& frames = Animations[state];

    if (FrameDuration <= 0.0f) return true;

    // 2. 시간 누적 및 인덱스 계산
    ElapsedTime += tick;
    int frameIndex = static_cast<int>(ElapsedTime / FrameDuration) % frames.size();
    bool isAnimationFinished = (ElapsedTime / FrameDuration) >= frames.size();

    // 3. Renderer에게 텍스처 포인터를 직접 전달 (매우 빠름)
    if (!isAnimationFinished) {
        _currentTexture = frames[frameIndex];
    }
    else
    {
        _currentTexture = frames[frames.size()-1];
    }
    renderer->SetTexture(_currentTexture);
	return isAnimationFinished;

}

void Animator::PlayRound(TextureRenderer* renderer, const std::string& state, float tick)
{
	ElapsedTime += tick;
    if (Animations.find(state) == Animations.end() || Animations[state].empty())
        return;

    std::vector<UTexture2D*>& frames = Animations[state];

    int frameCount = frames.size();
    int frameIndex;

    if (frameCount == 1)
    {
        frameIndex = 0;
    }
    else {
        int cycle = frameCount * 2 - 2;

        int index = static_cast<int>(ElapsedTime / FrameDuration) % cycle;


        if (index < frameCount)
            frameIndex = index;
        else
            frameIndex = cycle - index;
    }
    _currentTexture = frames[frameIndex];
    renderer->SetTexture(_currentTexture);
}

void Animator::AddFrames(const std::string& state, const std::vector<std::wstring>& filePaths)
{
    for (const std::wstring& path : filePaths) {
        UTexture2D* texture = UEngine::GetInstance().GetResourceManager().LoadTexture(path);
        if (texture)
        {
            Animations[state].push_back(texture);
        }

    }

}

