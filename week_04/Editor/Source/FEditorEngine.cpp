#include "FEditorEngine.h"

#include "UI/EditorViewportClient.h"
#include "UI/PreviewViewportClient.h"
#include "Core/Core.h"
#include "Core/ConsoleVariableManager.h"
#include "Scene/Scene.h"
#include "Actor/Actor.h"
#include "Camera/Camera.h"
#include "Component/CubeComponent.h"
#include "Object/ObjectFactory.h"
#include "Debug/EngineLog.h"

#include "imgui_impl_win32.h"

namespace
{
	constexpr const char* PreviewSceneContextName = "PreviewScene";

	void InitializeDefaultPreviewScene(CCore* Core)
	{
		if (Core == nullptr)
		{
			return;
		}

		FEditorSceneContext* PreviewContext = Core->CreatePreviewSceneContext(PreviewSceneContextName);
		if (PreviewContext == nullptr || PreviewContext->Scene == nullptr)
		{
			return;
		}

		UScene* PreviewScene = PreviewContext->Scene;
		if (PreviewScene->GetActors().empty())
		{
			AActor* PreviewActor = PreviewScene->SpawnActor<AActor>("PreviewCube");
			if (PreviewActor)
			{
				UCubeComponent* PreviewComponent = FObjectFactory::ConstructObject<UCubeComponent>(PreviewActor);
				PreviewActor->AddOwnedComponent(PreviewComponent);
				PreviewActor->SetActorLocation({ 0.0f, 0.0f, 0.0f });
			}
		}

		if (CCamera* PreviewCamera = PreviewScene->GetCamera())
		{
			PreviewCamera->SetPosition({ -8.0f, -8.0f, 6.0f });
			PreviewCamera->SetRotation(45.0f, -20.0f);
			PreviewCamera->SetFOV(50.0f);
		}
	}
}

bool FEditorEngine::Initialize(HINSTANCE hInstance)
{
	ImGui_ImplWin32_EnableDpiAwareness();

	if (!FEngine::Initialize(hInstance, L"Jungle Editor", 1280, 720))
	{
		return false;
	}

	return true;
}

FEditorEngine::~FEditorEngine()
{
	Shutdown();
}

void FEditorEngine::Shutdown()
{
	if (Core && Core->GetViewportClient() == PreviewViewportClient.get())
	{
		Core->SetViewportClient(nullptr);
	}

	PreviewViewportClient.reset();
	FEngine::Shutdown();
}

void FEditorEngine::PreInitialize()
{
	FEngineLog::Get().SetCallback([this](const char* Msg)
		{
			EditorUI.GetConsole().AddLog("%s", Msg);
		});
}

void FEditorEngine::PostInitialize()
{
	InitializeDefaultPreviewScene(Core.get());
	PreviewViewportClient = std::make_unique<CPreviewViewportClient>(EditorUI, MainWindow, PreviewSceneContextName);

	FConsoleVariableManager& CVM = FConsoleVariableManager::Get();
	TArray<FString> VariableNames;
	CVM.GetAllNames(VariableNames);
	for (const FString& Name : VariableNames)
	{
		EditorUI.GetConsole().RegisterCommand(Name.c_str());
	}

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

	SyncViewportClient();
	UE_LOG("EditorEngine initialized");
}

void FEditorEngine::Tick(float DeltaTime)
{
	SyncViewportClient();
}

std::unique_ptr<IViewportClient> FEditorEngine::CreateViewportClient()
{
	return std::make_unique<CEditorViewportClient>(EditorUI, MainWindow);
}

void FEditorEngine::SyncViewportClient()
{
	if (!Core)
	{
		return;
	}

	IViewportClient* TargetViewportClient = ViewportClient.get();
	const FSceneContext* ActiveSceneContext = Core->GetActiveSceneContext();
	if (ActiveSceneContext && ActiveSceneContext->SceneType == ESceneType::Preview && PreviewViewportClient)
	{
		TargetViewportClient = PreviewViewportClient.get();
	}

	if (Core->GetViewportClient() != TargetViewportClient)
	{
		Core->SetViewportClient(TargetViewportClient);
	}
}
