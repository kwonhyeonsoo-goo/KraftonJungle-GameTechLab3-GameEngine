#pragma once
#include "CoreMinimal.h"
#include "Scene/SceneTypes.h"
#include "Windows.h"
#include "Core/Core.h"
#include <memory>

class CWindowApplication;

class ENGINE_API FEngine
{
public:
	FEngine() = default;
	virtual ~FEngine();

	FEngine(const FEngine&) = delete;
	FEngine& operator=(const FEngine&) = delete;

	bool Initialize(HINSTANCE hInstance, const wchar_t* Title, int32 Width, int32 Height);
	void Run();
	virtual void Shutdown();

	CCore* GetCore() const { return Core.get(); }
	CWindowApplication* GetApp() const { return App; }

protected:
	virtual void Startup() {}
	virtual void Tick(float DeltaTime) {}
	virtual ESceneType GetStartupSceneType() const { return ESceneType::Game; }

	CWindowApplication* App = nullptr;
	std::unique_ptr<CCore> Core;

private:
	bool OnInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	void OnResize(int32 Width, int32 Height);
};
