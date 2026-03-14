#pragma once
#include "CoreMinimal.h"
#include "Windows.h"

class UScene;
class CRenderer;

class ENGINE_API CCore
{
public:
	CCore() = default;
	~CCore();

	CCore(const CCore&) = delete;
	CCore(CCore&&) = delete;
	CCore& operator=(const CCore&) = delete;
	CCore& operator=(CCore&&) = delete;

	bool Initialize(HWND Hwnd);
	void Release();

	void Tick(float DeltaTime);
private:
	void Physics(float DeltaTime);
	void GameLogic(float DeltaTime);
	void Render();

	void RegisterObjects();

private:
	CRenderer* Renderer = nullptr;
	UScene* Scene = nullptr;

	int32 WindowWidth = 0;
	int32 WindowHeight = 0;
};
