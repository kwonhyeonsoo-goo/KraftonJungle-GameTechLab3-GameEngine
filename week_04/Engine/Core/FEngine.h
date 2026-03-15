#pragma once
#include "CoreMinimal.h"
#include "Windows.h"

class CWindowApplication;
class CWindow;
class CCore;

class ENGINE_API FEngine
{
public:
	FEngine() = default;
	virtual ~FEngine();

	FEngine(const FEngine&) = delete;
	FEngine& operator=(const FEngine&) = delete;

	bool Initialize(HINSTANCE hInstance, const wchar_t* Title, int Width, int Height);
	void Run();
	virtual void Shutdown();

	CCore* GetCore() const { return Core; }
	CWindow* GetMainWindow() const { return MainWindow; }

protected:
	virtual void Startup() {}
	virtual void Tick(float DeltaTime) {}

	CWindowApplication* App = nullptr;
	CWindow* MainWindow = nullptr;
	CCore* Core = nullptr;
};
