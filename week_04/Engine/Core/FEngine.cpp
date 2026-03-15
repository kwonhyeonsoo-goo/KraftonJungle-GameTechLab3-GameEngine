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

	// Input forwarding
	MainWindow->AddMessageFilter([this](HWND h, UINT m, WPARAM w, LPARAM l) -> bool
		{
			if (Core)
			{
				Core->ProcessInput(h, m, w, l);
			}
			return false;
		});

	// Resize callback
	MainWindow->SetOnResizeCallback([this](int W, int H)
		{
			if (Core)
			{
				Core->OnResize(W, H);
			}
		});

	Startup();

	MainWindow->Show();

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
