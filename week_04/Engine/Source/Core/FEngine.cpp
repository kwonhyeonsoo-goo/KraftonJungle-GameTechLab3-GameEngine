#include "FEngine.h"
#include "Platform/Windows/WindowApplication.h"

FEngine* GEngine = nullptr;

FEngine::~FEngine()
{
	Shutdown();
}

bool FEngine::Initialize(HINSTANCE hInstance, const wchar_t* Title, int32 Width, int32 Height)
{
	App = &CWindowApplication::Get();
	if (!App->Create(hInstance))
		return false;

	if (!App->CreateMainWindow(Title, Width, Height))
		return false;
	GEngine = this;

	Core = std::make_unique<CCore>();
	if (!Core->Initialize(MainWindow->GetHwnd(), MainWindow->GetWidth(), MainWindow->GetHeight(), GetStartupSceneType()))
		return false;

	Startup();

	// Input forwarding (registered after Startup so Editor can add ImGui/Picking filters first)
	App->AddMessageFilter(std::bind(&FEngine::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	// Resize callback
	App->SetOnResizeCallback(std::bind(&FEngine::OnResize, this, std::placeholders::_1, std::placeholders::_2));

	App->ShowWindow();

	return true;
}

void FEngine::Run()
{
	while (App->PumpMessages())
	{
		Core->Tick();
	}
}

bool FEngine::OnInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (Core)
	{
		Core->ProcessInput(Hwnd, Msg, WParam, LParam);
	}
	return false;
}

void FEngine::OnResize(int32 Width, int32 Height)
{
	if (Core)
	{
		Core->OnResize(Width, Height);
	}
}

void FEngine::Shutdown()
{
	GEngine = nullptr;

	if (Core)
	{
		Core->Release();
		Core.reset();
	}

	if (App)
	{
		App->Destroy();
		App = nullptr;
	}
}
