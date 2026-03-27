#pragma once
#include "CoreMinimal.h"
#include "World/LevelTypes.h"
#include "Windows.h"
#include "Core/Core.h"
#include "ViewportClient.h"
#include <memory>

class FWindowApplication;
class FWindow;

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

	FCore* GetCore() const { return Core.get(); }
	FWindowApplication* GetApp() const { return App; }

protected:
	virtual void PreInitialize() {}
	virtual void PostInitialize() {}
	virtual void Tick(float DeltaTime) {}
	virtual ELevelType GetStartupLevelType() const { return ELevelType::Game; }
	virtual std::unique_ptr<IViewportClient> CreateViewportClient();

	FWindowApplication* App = nullptr;
	FWindow* MainWindow = nullptr;
	std::unique_ptr<FCore> Core;
	std::unique_ptr<IViewportClient> ViewportClient;

private:
	bool OnInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	void OnResize(int32 Width, int32 Height);
};
