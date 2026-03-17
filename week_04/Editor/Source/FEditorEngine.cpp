#include "FEditorEngine.h"
#include "Core/Core.h"
#include "Platform/Windows/WindowApplication.h"
#include "Debug/EngineLog.h"

#include "imgui_impl_win32.h"

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

	FEngineLog::Get().SetCallback([this](const char* Msg)
		{
			EditorUI.GetConsole().AddLog("%s", Msg);
		});
	UE_LOG("EditorEngine initialized");
}
