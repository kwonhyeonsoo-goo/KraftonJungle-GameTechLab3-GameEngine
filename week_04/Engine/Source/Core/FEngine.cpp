#include "FEngine.h"
#include "Platform/Windows/Window.h"
#include "Core/ViewportClient.h"
#include "Platform/Windows/WindowApplication.h"
#include "Object/ObjectGlobals.h"

FEngine* GEngine = nullptr;

FEngine::~FEngine()
{
	Shutdown();
}

bool FEngine::Initialize(HINSTANCE hInstance, const wchar_t* Title, int32 Width, int32 Height)
{
	App = &FWindowApplication::Get();
	if (!App->Create(hInstance))
	{
		return false;
	}

	if (!App->CreateMainWindow(Title, Width, Height))
	{
		return false;
	}

	GEngine = this;

	MainWindow = App->GetMainWindow();
	if (!MainWindow)
	{
		return false;
	}

	PreInitialize();

	Core = std::make_unique<FCore>();
	if (!Core->Initialize(MainWindow->GetHwnd(), MainWindow->GetWidth(), MainWindow->GetHeight(), GetStartupLevelType()))
	{
		return false;
	}

	CreateViewportClient();
	Core->SetViewportClient(ViewportClientArray);

	PostInitialize();

	App->AddMessageFilter(std::bind(&FEngine::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	App->SetOnResizeCallback(std::bind(&FEngine::OnResize, this, std::placeholders::_1, std::placeholders::_2));
	App->ShowWindow();

	return true;
}

void FEngine::Run()
{
	while (App->PumpMessages())
	{
		if (Core)
		{
			Tick(Core->GetTimer().GetDeltaTime());
			Core->Tick();
		}
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

void FEngine::CreateViewportClient()
{
	ViewportClientArray.reserve(5);
	for (uint32 i = 0; i < 5; i++) {
		ViewportClientArray.push_back(std::make_unique<FGameViewportClient>());
	}
}

void FEngine::Shutdown()
{
	GEngine = nullptr;

	if (Core)
	{
		//Core->SetViewportClient(nullptr);
		Core->Release();
		Core.reset();
	}

	for (auto& viewportClient : ViewportClientArray)
	{
		viewportClient.reset();
	}

	if (App)
	{
		App->Destroy();
		App = nullptr;
	}

	MainWindow = nullptr;
}
