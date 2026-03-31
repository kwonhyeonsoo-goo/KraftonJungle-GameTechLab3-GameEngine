#include "FEditorEngine.h"

#include "imgui_impl_dx11.h"
#include "UI/EditorViewportClient.h"
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
#include "Camera/Camera.h"
#include "Actor/SkySphereActor.h"

#include "Actor/StaticMeshActor.h"
#include "Asset/AssetManager.h"
#include "Utility/FileIO.h"

#include "../Source/Controller/OrthoViewportController.h"

bool FEditorEngine::Initialize(HINSTANCE hInstance)
{
	ImGui_ImplWin32_EnableDpiAwareness();
	return FEngine::Initialize(hInstance, L"Jungle Editor", 1280, 720);
}

FEditorEngine::~FEditorEngine() {}

void FEditorEngine::Shutdown()
{

	for (auto& viewportcontroller : ViewportControllerArray)
	{
		viewportcontroller.reset();
	}

	for (auto& VP : SceneViewportClients) VP = nullptr;
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
	// [0] Perspective — 주 에디터 뷰, Gizmo/Grid 포함
	// [1] Top         — 위에서 아래 orthographic
	// [2] Side        — 옆에서 orthographic
	// [3] Bottom      — 아래에서 위 orthographic
	ViewportControllerArray.reserve(4);


	auto VP = std::make_unique<FEditorViewportClient>(EditorUI, MainWindow);
	SceneViewportClients[0] = VP.get();

	auto ViewportController = std::make_unique<FViewportController>();
	ViewportControllerArray.push_back(std::move(ViewportController));
	ViewportClientArray.push_back(std::move(VP));


	for (int i = 1; i < 4; i++)
	{
		auto VP = std::make_unique<FEditorViewportClient>(EditorUI, MainWindow);
		SceneViewportClients[i] = VP.get();
		
		auto ViewportController = std::make_unique<FOrthoViewportController>();
		ViewportControllerArray.push_back(std::move(ViewportController));
		ViewportClientArray.push_back(std::move(VP));
	}
}

void FEditorEngine::PostInitialize()
{
	// ── FViewport ↔ IViewportClient 1:1 연결 ─────────────────────────
	for (int i = 0; i < 4; i++)
	{
		EditorUI.LinkViewportClient(i, SceneViewportClients[i]);
		SceneViewportClients[i]->SetLinkedWorld(Core->GetEditorWorld() , Core.get());
		SceneViewportClients[i]->InitializeCameraFromWorld();
	}

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

	{
		UCameraComponent* Cam = SceneViewportClients[0]->GetActiveCamera();
		Cam->SetPosition({ -10.f, 0.f, 5.f });
		Cam->SetRotation(0.f, -15.f);
		Cam->SetFOV(60.f);
		Cam->SetOrthographic(false);
		ViewportControllerArray[0].get()->Initialize(Cam, Core->GetInputManager());

	}
	UE_LOG("EditorEngine initialized");

	// [1] Top — 위에서 아래로 내려다봄
	{
		UCameraComponent* Cam = SceneViewportClients[1]->GetActiveCamera();
		Cam->SetPosition({ 0.f, 0.f, 100.f });
		Cam->SetRotation(0.f, -90.f);   // 거의 수직 아래
		Cam->SetOrthographic(true);
		Cam->SetOrthoWidth(15.f);
		ViewportControllerArray[1].get()->Initialize(Cam, Core->GetInputManager());

	}

	// [2] Side — 오른쪽에서 왼쪽으로
	{
		UCameraComponent* Cam = SceneViewportClients[2]->GetActiveCamera();
		Cam->SetPosition({ 100.f, 0.f, 2.f });
		Cam->SetRotation(180.f, 0.f);    // 왼쪽 방향
		Cam->SetOrthographic(true);
		Cam->SetOrthoWidth(15.f);
		ViewportControllerArray[2].get()->Initialize(Cam, Core->GetInputManager());

	}

	// [3] Bottom — 아래에서 위로 올려다봄 (Front 뷰로 대체하는 경우 많음)
	{
		UCameraComponent* Cam = SceneViewportClients[3]->GetActiveCamera();
		Cam->SetPosition({ 0.f, -100.f, 2.f });
		Cam->SetRotation(90.f, 0.f);     // 앞쪽 방향
		Cam->SetOrthographic(true);
		Cam->SetOrthoWidth(15.f);
		ViewportControllerArray[3].get()->Initialize(Cam, Core->GetInputManager());

	}

	// ── ViewportController — Perspective 뷰포트만 제어 ───────────────


#if IS_OBJ_VIEWER
	if (Core && Core->GetActiveLevel())
	{
		FString Path = GetObjFilePath(EFileDialogType::Open);

		if (!Path.empty())
		{
			if (!Path.ends_with(".obj"))
			{
				MessageBoxW(
					nullptr,\
					L"Obj 파일만 지원합니다.",
					L"Error",
					MB_OK | MB_ICONWARNING
				);

				assert(false);
			}

			AStaticMeshActor* NewActor = Core->GetLevel()->SpawnActor<AStaticMeshActor>("StaticMeshActor");
			UStaticMesh* StaticMesh = FAssetManager::LoadObjStaticMesh(FPaths::ToRelativePath(Path), Core->GetRenderer()->GetDevice());
			NewActor->SetStaticMesh(StaticMesh);
			ViewportControllerArray[0].get()->SetFocus(NewActor->GetRootComponent());

		}
		else
		{
			MessageBoxW(
				nullptr,
				L"파일이 선택되지 않았습니다.",
				L"Error",
				MB_OK | MB_ICONWARNING
			);

			assert(false);
		}
	}

#else

#endif

	UE_LOG("EditorEngine initialized");



}


void FEditorEngine::Tick(float DeltaTime)
{
	int32 FocusedIndex = EditorUI.GetHoveredViewportIndex();

	for (int i = 0; i < 4; i++)
	{
		bool bShouldBeActive = (i == FocusedIndex);
		ViewportControllerArray[i]->SetActive(bShouldBeActive);
		ViewportControllerArray[i]->Tick(DeltaTime);
	}
}

