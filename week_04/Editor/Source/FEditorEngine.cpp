#include "FEditorEngine.h"
#include "Core/Core.h"
#include "Scene/Scene.h"
#include "Actor/Actor.h"
#include "Camera/Camera.h"
#include "Component/CubeComponent.h"
#include "Platform/Windows/WindowApplication.h"
#include "Debug/EngineLog.h"

#include "imgui_impl_win32.h"

namespace
{
	void InitializeDefaultPreviewScene(CCore* Core)
	{
		if (Core == nullptr)
		{
			return;
		}

		FEditorSceneContext* PreviewContext = Core->CreatePreviewSceneContext("PreviewScene");
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
				UPrimitiveComponent* PreviewComponent = new UCubeComponent();
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
		return false;

	return true;
}

void FEditorEngine::Startup()
{
	EditorUI.Initialize(Core.get());
	EditorUI.SetupWindow(App->GetMainWindow());
	InitializeDefaultPreviewScene(Core);

	FEngineLog::Get().SetCallback([this](const char* Msg)
		{
			EditorUI.GetConsole().AddLog("%s", Msg);
		});
	UE_LOG("EditorEngine initialized");
}
