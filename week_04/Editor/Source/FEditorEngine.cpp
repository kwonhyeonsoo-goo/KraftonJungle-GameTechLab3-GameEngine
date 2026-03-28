#include "FEditorEngine.h"

#include "imgui_impl_dx11.h"
#include "UI/EditorViewportClient.h"
#include "UI/PreviewViewportClient.h"
#include "Core/Core.h"
#include "Core/ConsoleVariableManager.h"
#include "World/Level.h"
#include "Actor/Actor.h"
#include "Component/CameraComponent.h"
#include "Component/CubeComponent.h"
#include "Object/ObjectFactory.h"
#include "Debug/EngineLog.h"
#include "World/World.h"
#include "imgui_impl_win32.h"
#include "Pawn/EditorCameraPawn.h"
#include "Actor/SkySphereActor.h"

namespace
{
	constexpr const char* PreviewLevelContextName = "PreviewLevel";

	void InitializeDefaultPreviewLevel(FCore* Core)
	{
		if (!Core) return;

		FEditorWorldContext* PreviewContext = Core->GetLevelManager()->CreatePreviewWorldContext(PreviewLevelContextName, 1280, 720);
		if (!PreviewContext || !PreviewContext->World) return;

		UWorld* PreviewWorld = PreviewContext->World;
		if (PreviewWorld->GetActors().empty())
		{
			AActor* PreviewActor = PreviewWorld->SpawnActor<AActor>("PreviewCube");
			if (PreviewActor)
			{
				UCubeComponent* PreviewComponent = FObjectFactory::ConstructObject<UCubeComponent>(PreviewActor);
				PreviewActor->AddOwnedComponent(PreviewComponent);
				PreviewActor->SetActorLocation({ 0.0f, 0.0f, 0.0f });
			}
		}
	}
}

bool FEditorEngine::Initialize(HINSTANCE hInstance)
{
	ImGui_ImplWin32_EnableDpiAwareness();
	return FEngine::Initialize(hInstance, L"Jungle Editor", 1280, 720);
}

FEditorEngine::~FEditorEngine()
{
}

void FEditorEngine::Shutdown()
{
	if (EditorPawn)
	{
		EditorPawn->Destroy();
		EditorPawn = nullptr;
	}

	ViewportController.Cleanup();

	// EditorViewportClient / PreviewViewportClient 포인터는
	// ViewportClientArray가 소유 — FEngine::Shutdown()에서 정리됨
	EditorViewportClient = nullptr;
	PreviewViewportClient = nullptr;

	FEngine::Shutdown();
}

void FEditorEngine::PreInitialize()
{
	FEngineLog::Get().SetCallback([this](const char* Msg)
		{
			EditorUI.GetConsole().AddLog("%s", Msg);
		});
}

void FEditorEngine::CreateViewportClients()
{
	// EditorViewportClient
	auto EditorVP = std::make_unique<FEditorViewportClient>(EditorUI, MainWindow);
	EditorViewportClient = EditorVP.get();
	ViewportClientArray.push_back(std::move(EditorVP));

	// PreviewViewportClient
	auto PreviewVP = std::make_unique<FPreviewViewportClient>(EditorUI, MainWindow, PreviewLevelContextName);
	PreviewViewportClient = PreviewVP.get();
	ViewportClientArray.push_back(std::move(PreviewVP));
}

void FEditorEngine::PostInitialize()
{
	InitializeDefaultPreviewLevel(Core.get());

	FConsoleVariableManager& CVM = FConsoleVariableManager::Get();
	CVM.GetAllNames([this](const FString& Name)
		{
			EditorUI.GetConsole().RegisterCommand(Name.c_str());
		});

	EditorUI.GetConsole().SetCommandHandler([](const char* CommandLine)
		{
			FString Result;
			if (FConsoleVariableManager::Get().Execute(CommandLine, Result))
			{
				FEngineLog::Get().Log("%s", Result.c_str());
			}
			else
			{
				FEngineLog::Get().Log("[error] Unknown command: '%s'", CommandLine);
			}
		});

	// EditorPawn 생성 및 카메라를 EditorViewportClient에 연결
	EditorPawn = FObjectFactory::ConstructObject<AEditorCameraPawn>(nullptr, "EditorCameraPawn");
	EditorPawn->Initialize();

	UCameraComponent* EditorCamera = EditorPawn->GetCameraComponent();
	if (EditorViewportClient)
	{
		EditorViewportClient->SetActiveCamera(EditorCamera);
	}

	// PreviewViewportClient는 PreviewWorld의 카메라를 사용
	if (PreviewViewportClient)
	{
		FEditorWorldContext* PreviewContext =
			Core->GetLevelManager()->FindPreviewWorldContext(PreviewLevelContextName);
		if (PreviewContext && PreviewContext->World)
		{
			UCameraComponent* PreviewCamera = PreviewContext->World->GetActiveCameraComponent();
			if (PreviewCamera)
			{
				PreviewCamera->SetPosition({ -8.0f, -8.0f, 6.0f });
				PreviewCamera->SetRotation(45.0f, -20.0f);
				PreviewCamera->SetFOV(50.0f);
				PreviewViewportClient->SetActiveCamera(PreviewCamera);
			}
		}
	}

	ViewportController.Initialize(
		EditorCamera,
		Core->GetInputManager(),
		Core->GetEnhancedInputManager());

	UE_LOG("EditorEngine initialized");
}

void FEditorEngine::Tick(float DeltaTime)
{
	ViewportController.Tick(DeltaTime);
}

FEditorViewportController* FEditorEngine::GetViewportController()
{
	return &ViewportController;
}