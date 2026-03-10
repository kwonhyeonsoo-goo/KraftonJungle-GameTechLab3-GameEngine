#pragma once
#include <Windows.h>

#include "UPrimitive.h"
#include "URenderer.h"
#include "UResourceManager.h"
#include "USceneManager.h"

class UEngine : public UPrimitive
{
public:
	bool Initialize(HWND hWnd, const std::string& startSceneName);
	void Release();

	URenderer& GetRenderer() { return Renderer; }
	UResourceManager& GetResourceManager() { return ResourceManager; }
	USceneManager& GetSceneManager() { return SceneManager; }

private:
	URenderer Renderer;
	UResourceManager ResourceManager;
	USceneManager SceneManager;
};

