#include "UGameApp.h"

#include <chrono>
#include <utility>

#include "USceneManager.h"
#include "URenderer.h"
#include "Utility.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

UGameApp::UGameApp(HINSTANCE hInst, FWindowDesc desc) : UWindow(hInst, std::move(desc)), Engine(nullptr)
{
}

UGameApp::~UGameApp() = default;

int UGameApp::Run(int nShowCmd)
{
	Show(nShowCmd);

	Initialize();

	using clock = std::chrono::steady_clock;
	auto prev = clock::now();

	MSG msg{};
	bool bIsRunning = true;

	while (bIsRunning)
	{
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				bIsRunning = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		if (!bIsRunning) break;

		auto now = clock::now();
		std::chrono::duration<float> dt = now - prev;
		prev = now;

		Tick(dt.count());
	}

	Shutdown();

	return static_cast<int>(msg.wParam);
}

void UGameApp::Initialize()
{
	// TODO : DX 초기화 같은 동작 수행
	Engine = &UEngine::GetInstance();
	Engine->Initialize(Handle(), "UWeek0Scene");

	auto& renderer = Engine->GetRenderer();

	// ImGui 초기화
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)Handle());
	ImGui_ImplDX11_Init(renderer.GetDevice(), renderer.GetDeviceContext());
}

void UGameApp::Tick(const float dt)
{
	// TODO : Update / Render 추가
	auto& SceneManager = Engine->GetSceneManager();
	auto& Renderer = Engine->GetRenderer();

	SceneManager.Update(dt);
	Renderer.Prepare();
	SceneManager.Render();
	EditorUpdate(dt);
	Renderer.SwapBuffer();
	SceneManager.ProcessPendingSceneChange();
}

void UGameApp::Shutdown()
{
	// TODO : 리소스 해제
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	Engine->Release();
}

void UGameApp::OnDestroy()
{
	UWindow::OnDestroy();
}

LRESULT UGameApp::OnMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	return UWindow::OnMessage(msg, wp, lp);
}

void UGameApp::EditorUpdate(float dt)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 이후 ImGui UI 컨트롤 추가는 ImGui::NewFrame()과 ImGui::Render()
	// 사이인 이곳에 위치
	ImGui::Begin("Jungle Property Window");
	ImGui::Text("Hello Jungle World!");
	ImGui::Text("Delta Time: %.6f", dt);   // 초 단위
	ImGui::Text("Frame Time: %.3f ms", dt * 1000.0f); // 밀리초
	ImGui::Text("FPS: %.1f", 1.0f / dt);   // FPS
	ImGui::End();

	DrawSceneManagerPanel();

	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void UGameApp::DrawSceneManagerPanel()
{
	if (!ImGui::Begin("Scene Manager"))
	{
		ImGui::End();
		return;
	}

	USceneManager* SceneManager = &Engine->GetSceneManager();

	const char* CurrentSceneLabel =
		SceneManager->HasCurrentScene() ? SceneManager->GetCurrentSceneName().c_str() : "<None>";

	ImGui::Text("Current Scene: %s", CurrentSceneLabel);

	ImGui::Separator();

	const auto& Entries = SceneRegistry::Get().GetEntries();
	const char* PreviewLabel =
		SceneManager->HasCurrentScene() ? SceneManager->GetCurrentSceneName().c_str() : "<Select Scene>";

	if (ImGui::BeginCombo("Scene", PreviewLabel))
	{
		for (const auto& Entry : Entries)
		{
			const bool bSelected = SceneManager->IsCurrentScene(Entry.Name);

			if (ImGui::Selectable(Entry.Name.c_str(), bSelected))
			{
				if (!bSelected)
				{
					SceneManager->RequestChangeScene(Entry.Name);
				}
			}

			if (bSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

	ImGui::Separator();

	if (ImGui::Button("Reload Current Scene"))
	{
		if (SceneManager->HasCurrentScene())
		{
			SceneManager->RequestChangeScene(SceneManager->GetCurrentSceneName());
		}
	}

	ImGui::Text("Registered Scene Count: %d", static_cast<int>(Entries.size()));

	ImGui::End();
}
