#pragma once
#include "CoreMinimal.h"
#include "Windows.h"

class UScene;
class CRenderer;
class CShaderManager;

class ENGINE_API CCore
{
public:
	CCore() = default;
	~CCore();

	CCore(const CCore&) = delete;
	CCore(CCore&&) = delete;
	CCore& operator=(const CCore&) = delete;
	CCore& operator=(CCore&&) = delete;

	bool Initialize(HWND Hwnd, int Width, int Height);
	void Release();

	void Tick(float DeltaTime);

	UScene* GetScene() const { return Scene; }
	CRenderer* GetRenderer() const { return Renderer; }

private:
	void Physics(float DeltaTime);
	void GameLogic(float DeltaTime);
	void Render();

private:
	CRenderer* Renderer = nullptr;
	CShaderManager* ShaderManager = nullptr;
	UScene* Scene = nullptr;

	int32 WindowWidth = 0;
	int32 WindowHeight = 0;
};
