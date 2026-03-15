#include "FEngine.h"
#include "Core.h"
#include "Platform/Windows/WindowApplication.h"
#include "Platform/Windows/Window.h"
#include "Renderer/Renderer.h"

FEngine::~FEngine()
{
	Shutdown();
}

bool FEngine::Initialize(HINSTANCE hInstance, const wchar_t* Title, int Width, int Height)
{
	App = &CWindowApplication::Get();
	if (!App->Create(hInstance))
		return false;

	MainWindow = App->MakeWindow(Title, Width, Height);
	if (!MainWindow)
		return false;

	Core = new CCore();
	if (!Core->Initialize(MainWindow->GetHwnd(), MainWindow->GetWidth(), MainWindow->GetHeight()))
		return false;

	Startup();

	return true;
}

void FEngine::Run()
{
	while (App->PumpMessages())
	{
		if (Core->GetRenderer()->IsOccluded())
			continue;

		Core->Tick();
	}
}

void FEngine::Shutdown()
{
	if (Core)
	{
		Core->Release();
		delete Core;
		Core = nullptr;
	}

	if (MainWindow)
	{
		delete MainWindow;
		MainWindow = nullptr;
	}

	if (App)
	{
		App->Destroy();
		App = nullptr;
	}
}
