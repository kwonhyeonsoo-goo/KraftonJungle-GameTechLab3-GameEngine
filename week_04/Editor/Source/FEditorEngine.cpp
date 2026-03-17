#include "FEditorEngine.h"
#include "Core/Core.h"
#include "Core/ConsoleVariableManager.h"
#include "Platform/Windows/WindowApplication.h"
#include "Debug/EngineLog.h"

#include "imgui_impl_win32.h"

bool FEditorEngine::Initialize(HINSTANCE hInstance)
{

	ImGui_ImplWin32_EnableDpiAwareness();

	if (!FEngine::Initialize(hInstance, L"Jungle Editor", 1280, 720))
		return false;

}

void FEditorEngine::Tick(float DeltaTime)
{
	ViewportController.Tick(DeltaTime);

	FEngine::Tick(DeltaTime);
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
	EditorUI.Initialize(Core.get());
	EditorUI.SetupWindow(App->GetMainWindow());

	// Console variable commands
	FConsoleVariableManager& CVM = FConsoleVariableManager::Get();
	TArray<FString> VarNames;
	CVM.GetAllNames(VarNames);
	for (const FString& Name : VarNames)
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
	EditorPawn = new AEditorCameraPawn(
		AEditorCameraPawn::StaticClass(), "EditorCameraPawn");
	Core->GetScene()->RegisterActor(EditorPawn);
	Core->GetScene()->SetActiveCameraComponent(EditorPawn->GetCameraComponent());

	ViewportController.Initialize(
		EditorPawn->GetCameraComponent(),
		Core->GetInputManager());

	UE_LOG("EditorEngine initialized");
}
