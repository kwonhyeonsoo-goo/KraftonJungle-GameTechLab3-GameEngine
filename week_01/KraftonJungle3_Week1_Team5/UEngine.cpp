#include "UEngine.h"

IMPLEMENT_SINGLETON(UEngine)

UEngine::UEngine() = default;

UEngine::~UEngine() = default;

bool UEngine::Initialize(HWND hWnd, const std::string& startSceneName)
{
	SoundManager.Initialize();

	if (!Renderer.Create(hWnd))
	{
		return false;
	}

	if (!ResourceManager.Initialize(Renderer.GetDevice()))
	{
		return false;
	}

	if (!SceneManager.Initialize(startSceneName, Renderer.GetDevice(), Renderer.GetDeviceContext()))
	{
		return false;
	}

	return true;
}

void UEngine::Release()
{
	SoundManager.Release();
	SceneManager.Shutdown();
	ResourceManager.Release();
	Renderer.Release();
}
