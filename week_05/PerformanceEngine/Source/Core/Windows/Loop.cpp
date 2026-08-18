#include "Loop.h"

#include "WindowsApplication.h"
#include "WindowsWindow.h"
#include "Core/Core.h"

FLoop::~FLoop() = default;

bool FLoop::PreInit(HINSTANCE HInstance, const FLaunchConfig& InConfig)
{
	// 이후 시작 단계가 같은 설정값을 보도록 실행 설정을 먼저 저장한다.
	Config = InConfig;
	bExitRequested = false;

	// 엔진 객체보다 먼저 호스트 앱과 메인 윈도우를 만든다.
	if (!InitializeApplication(HInstance))
	{
		return false;
	}

	// Launch 설정에 맞는 구체적인 엔진 인스턴스를 만든다.
	return CreateCoreInstance();
}

bool FLoop::Init()
{
	// 생성된 호스트 컨텍스트를 엔진에 넘겨 런타임을 초기화한다.
	return InitializeCore();
}

void FLoop::Tick()
{
	// 호스트 앱이나 엔진이 없으면 더 진행할 수 없으므로 종료를 요청한다.
	if (!App || !Core)
	{
		RequestExit();
		return;
	}

	// 먼저 Windows 메시지 큐를 처리한다. false면 창이 종료를 요청한 상태다.
	if (!App->PumpMessages())
	{
		RequestExit();
		return;
	}

	// 호스트 메시지 처리가 끝난 뒤 엔진 프레임을 한 번 실행한다.
	Core->Tick();
}

void FLoop::Exit()
{
	if (Core)
	{
		Core->Shutdown();
		Core.reset();
	}

	if (App)
	{
		App->Destroy();
		App = nullptr;
	}

	MainWindow = nullptr;
}

void FLoop::RequestExit()
{
	bExitRequested = true;
}

bool FLoop::IsExitRequested() const
{
	return bExitRequested;
}

bool FLoop::InitializeApplication(HINSTANCE hInstance)
{
	App = &FWindowsApplication::Get();
	if (!App->Create(hInstance))
	{
		return false;
	}

	if (!App->CreateMainWindow(Config.Title, Config.Width, Config.Height))
	{
		return false;
	}

	MainWindow = App->GetMainWindow();
	if (MainWindow == nullptr)
	{
		return false;
	}

	App->ShowWindow();
	return true;
}

bool FLoop::CreateCoreInstance()
{
	Core = std::make_unique<FCore>();

	return Core != nullptr;
}

bool FLoop::InitializeCore() const
{
	if (!Core || !MainWindow)
	{
		return false;
	}

	FCoreInitArgs InitArgs;
	InitArgs.MainWindow = MainWindow;
	InitArgs.Hwnd = MainWindow->GetHwnd();
	InitArgs.Width = MainWindow->GetWidth();
	InitArgs.Height = MainWindow->GetHeight();

	if (!Core->Initialize(InitArgs))
	{
		return false;
	}

	App->AddMessageFilter(std::bind(&FCore::HandleMessage, Core.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	App->SetOnResizeCallback(std::bind(&FCore::HandleResize, Core.get(), std::placeholders::_1, std::placeholders::_2));

	return true;
}
