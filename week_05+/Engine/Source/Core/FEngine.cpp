#include "FEngine.h"
#include "Platform/Windows/Window.h"
#include "Platform/Windows/WindowApplication.h"
#include "Object/ObjectGlobals.h"
#include "Math/Rect.h"

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

	Timer = new FTimer();

	PreInitialize();

	Core = std::make_unique<FCore>();
	if (!Core->Initialize(MainWindow->GetHwnd(), MainWindow->GetWidth(), MainWindow->GetHeight(), GetStartupLevelType()))
	{
		return false;
	}

	InputManager = new FInputManager();
	EnhancedInput = new FEnhancedInputManager();
	PostInitialize();

	App->AddMessageFilter(std::bind(&FEngine::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	App->SetOnResizeCallback(std::bind(&FEngine::OnResize, this, std::placeholders::_1, std::placeholders::_2));
	App->ShowWindow();

	return true;
}

FViewportContext* FEngine::CreateContext(FRect InRect)
{
	auto ViewportClient = CreateViewportClient();
	auto Viewport = new FViewport(InRect);
	
	FViewportContext* ViewportContext = new FViewportContext(Viewport, ViewportClient);
	ViewportContext->Initialize(InputManager, EnhancedInput);
	return ViewportContext;
}

FWorldContext& FEngine::CreateWorldContext(EWorldType WorldType, UWorld* InWorld)
{
	FWorldContext NewContext;
	NewContext.ContextName = "";
	NewContext.WorldType = WorldType;
	NewContext.World = InWorld;

	if (!NewContext.World)
	{
		UWorld* World = FObjectFactory::ConstructObject<UWorld>(nullptr, NewContext.ContextName);
		World->SetWorldType(WorldType);
		World->InitializeWorld();

		NewContext.World = World;
	}

	return GEngine->AddWorldContext(NewContext);
}

void FEngine::Run()
{
	while (App->PumpMessages())
	{
		Timer->Tick();
		Tick(Timer->GetDeltaTime());
	}
}

bool FEngine::OnInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	ProcessInput(Hwnd, Msg, WParam, LParam);
	return false;
}

void FEngine::OnResize(int32 Width, int32 Height)
{
	if (Core)
	{
		Core->OnResize(Width, Height);
	}
	OnMainWindowResized(Width, Height);
}

void FEngine::Input(float DeltaTime)
{
	if (InputManager)
	{
		InputManager->Tick();
	}

	if (EnhancedInput && InputManager)
	{
		EnhancedInput->ProcessInput(InputManager, DeltaTime);
	}
}

void FEngine::ProcessInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (InputManager)
	{
		InputManager->ProcessMessage(Hwnd, Msg, WParam, LParam);
	}
}

void FEngine::Tick(float DeltaTime)
{
	Timer->Tick();
}

void FEngine::Shutdown()
{
	GEngine = nullptr;

	if (Core)
	{
		Core->Release();
		Core.reset();
	}

	delete EnhancedInput;
	EnhancedInput = nullptr;
	delete InputManager;
	InputManager = nullptr;

	if (App)
	{
		App->Destroy();
		App = nullptr;
	}

	MainWindow = nullptr;
}

void FEngine::Render()
{
	if (!Core)
	{
		return;
	}

	if (!GRenderer || GRenderer->IsOccluded())
	{
		return;
	}

	GRenderer->BeginFrame();
	GRenderer->EndFrame();
}
