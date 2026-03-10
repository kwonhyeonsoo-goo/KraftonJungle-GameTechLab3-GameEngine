#include "UEngine.h"

IMPLEMENT_SINGLETON(UEngine)

UEngine::UEngine() = default;

UEngine::~UEngine() = default;

bool UEngine::Initialize(HWND hWnd, const std::string& startSceneName)
{
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
	SceneManager.Shutdown();
	ResourceManager.Release();
	Renderer.Release();
}
