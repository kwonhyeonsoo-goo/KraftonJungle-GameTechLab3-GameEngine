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

namespace
{
	constexpr const char* PreviewLevelContextName = "PreviewLevel";

	void InitializeDefaultPreviewLevel(FCore* Core)
	{
		if (!Core) return;
		FEditorWorldContext* PreviewContext =
			Core->GetLevelManager()->CreatePreviewWorldContext(PreviewLevelContextName, 1280, 720);
		if (!PreviewContext || !PreviewContext->World) return;

		UWorld* PreviewWorld = PreviewContext->World;
		if (PreviewWorld->GetActors().empty())
		{
			AActor* PreviewActor = PreviewWorld->SpawnActor<AActor>("PreviewCube");
			if (PreviewActor)
			{
				UCubeComponent* Comp = FObjectFactory::ConstructObject<UCubeComponent>(PreviewActor);
				PreviewActor->AddOwnedComponent(Comp);
				PreviewActor->SetActorLocation({ 0.f, 0.f, 0.f });
			}
		}
	}
}

bool FEditorEngine::Initialize(HINSTANCE hInstance)
{
	ImGui_ImplWin32_EnableDpiAwareness();
	return FEngine::Initialize(hInstance, L"Jungle Editor", 1280, 720);
}

FEditorEngine::~FEditorEngine() {}

void FEditorEngine::Shutdown()
{
	if (EditorPawn) { EditorPawn->Destroy(); EditorPawn = nullptr; }
	ViewportController.Cleanup();
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
	// [0] EditorViewportClient
	auto EditorVP = std::make_unique<FEditorViewportClient>(EditorUI, MainWindow);
	EditorViewportClient = EditorVP.get();
	ViewportClientArray.push_back(std::move(EditorVP));

	// [1] PreviewViewportClient
	auto PreviewVP = std::make_unique<FPreviewViewportClient>(EditorUI, MainWindow, PreviewLevelContextName);
	PreviewViewportClient = PreviewVP.get();
	ViewportClientArray.push_back(std::move(PreviewVP));
}

void FEditorEngine::PostInitialize()
{
	InitializeDefaultPreviewLevel(Core.get());

	// ── FViewport ↔ IViewportClient 1:1 연결 ─────────────────────────
	EditorUI.LinkViewportClient(0, EditorViewportClient);
	EditorUI.LinkViewportClient(1, PreviewViewportClient);

	// ── Console ───────────────────────────────────────────────────────
	FConsoleVariableManager& CVM = FConsoleVariableManager::Get();
	CVM.GetAllNames([this](const FString& Name)
		{
			EditorUI.GetConsole().RegisterCommand(Name.c_str());
		});
	EditorUI.GetConsole().SetCommandHandler([](const char* CommandLine)
		{
			FString Result;
			if (FConsoleVariableManager::Get().Execute(CommandLine, Result))
				FEngineLog::Get().Log("%s", Result.c_str());
			else
				FEngineLog::Get().Log("[error] Unknown command: '%s'", CommandLine);
		});

	// ── EditorPawn / 카메라 연결 ──────────────────────────────────────
	EditorPawn = FObjectFactory::ConstructObject<AEditorCameraPawn>(nullptr, "EditorCameraPawn");
	EditorPawn->Initialize();

	UCameraComponent* EditorCamera = EditorPawn->GetCameraComponent();
	if (EditorViewportClient)
	{
		EditorViewportClient->SetActiveCamera(EditorCamera);
	}

	// PreviewViewportClient — World가 카메라를 소유하지 않으므로
	// ViewportClient의 DefaultCamera를 직접 초기화
	// World에 CameraActor를 배치하는 방식은 추후 확장 예정
	if (PreviewViewportClient)
	{
		UCameraComponent* PreviewCamera = PreviewViewportClient->GetActiveCamera();
		if (PreviewCamera)
		{
			PreviewCamera->SetPosition({ -8.f, -8.f, 6.f });
			PreviewCamera->SetRotation(45.f, -20.f);
			PreviewCamera->SetFOV(50.f);
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