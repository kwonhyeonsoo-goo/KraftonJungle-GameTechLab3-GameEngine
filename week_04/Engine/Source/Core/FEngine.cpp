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

	// 서브클래스가 ViewportClientArray를 채운 뒤 Core에 등록
	CreateViewportClients();
	Core->SetViewportClients(ViewportClientArray);

	PostInitialize();

	App->AddMessageFilter(std::bind(&FEngine::OnInput, this,
		std::placeholders::_1, std::placeholders::_2,
		std::placeholders::_3, std::placeholders::_4));
	App->SetOnResizeCallback(std::bind(&FEngine::OnResize, this,
		std::placeholders::_1, std::placeholders::_2));
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

void FEngine::CreateViewportClients()
{
	// 기본 구현: FGameViewportClient 하나
	// 서브클래스(FEditorEngine 등)에서 override해서 필요한 ViewportClient 추가
	ViewportClientArray.push_back(std::make_unique<FGameViewportClient>());
}

void FEngine::Shutdown()
{
	GEngine = nullptr;

	if (Core)
	{
		Core->Release();
		Core.reset();
	}

	// Core 해제 후 소유권 반환
	ViewportClientArray.clear();

	if (App)
	{
		App->Destroy();
		App = nullptr;
	}

	MainWindow = nullptr;
}