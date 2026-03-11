#pragma once
#include <Windows.h>

#include "UPrimitive.h"
#include "URenderer.h"
#include "UResourceManager.h"
#include "USceneManager.h"
#include "USoundManager.h"

class UEngine : public UPrimitive
{
private:
	UEngine();
	~UEngine() override;
public:
	DECLARE_SINGLETON(UEngine)

public:
	bool Initialize(HWND hWnd, const std::string& startSceneName);
	void Release();

	URenderer& GetRenderer() { return Renderer; }
	UResourceManager& GetResourceManager() { return ResourceManager; }
	USceneManager& GetSceneManager() { return SceneManager; }
	USoundManager& GetSoundManager() { return SoundManager; }

private:
	URenderer Renderer;
	UResourceManager ResourceManager;
	USceneManager SceneManager;
	USoundManager SoundManager;
};

