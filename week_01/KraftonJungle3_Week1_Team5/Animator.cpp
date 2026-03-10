#include "Animator.h"

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
void Animator::Play(TextureRenderer* renderer, const std::string& state, float tick)
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

