#pragma once
#include <memory>

#include "Core/Core.h"
#include "WindowsLaunch.h"
class FWindowsWindow;
class FWindowsApplication;

class FLoop
{
public:
	FLoop() = default;
	~FLoop();

	FLoop(const FLoop&) = delete;
	FLoop(FLoop&&) = delete;
	FLoop& operator=(const FLoop&) = delete;
	FLoop& operator=(FLoop&&) = delete;

	bool PreInit(HINSTANCE HInstance, const FLaunchConfig& InConfig);

	bool Init();

	void Tick();

	void Exit();

	void RequestExit();

	bool IsExitRequested() const;

	FCore* GetCore() const { return Core.get(); }
	FWindowsApplication* GetApp() const { return App; }
	FWindowsWindow* GetMainWindow() const { return MainWindow; }

private:
	bool InitializeApplication(HINSTANCE hInstance);
	bool CreateCoreInstance();
	bool InitializeCore() const;

private:
	FLaunchConfig Config;
	bool bExitRequested = false;

	FWindowsApplication* App = nullptr;
	FWindowsWindow* MainWindow = nullptr;
	std::unique_ptr<FCore> Core;

};

