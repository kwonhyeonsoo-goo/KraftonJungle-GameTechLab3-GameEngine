#include "UGameApp.h"

#include <chrono>
#include <string>
#include <utility>

#include "UGameObject.h"
#include "UScene.h"
#include "USceneManager.h"
#include "URenderer.h"
#include "Utility.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "UGameManager.h"

UGameApp::UGameApp(HINSTANCE hInst, FWindowDesc desc) : UWindow(hInst, std::move(desc)), Engine(nullptr)
{
}

UGameApp::~UGameApp() = default;

int UGameApp::Run(int nShowCmd)
{
	Show(nShowCmd);

	if (!Initialize())
	{
		return -1;
	}

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

bool UGameApp::Initialize()
{
	// TODO : DX 초기화 같은 동작 수행
	Engine = &UEngine::GetInstance();
	//Engine->Initialize(Handle(), "UWeek0Scene");
	if (!Engine->Initialize(Handle(), "UMainTitleScene"))
	{
		return false;
	}

	auto& renderer = Engine->GetRenderer();

	// ImGui 초기화
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)Handle());
	ImGui_ImplDX11_Init(renderer.GetDevice(), renderer.GetDeviceContext());

	return true;
}

void UGameApp::Tick(const float dt)
{
	// TODO : Update / Render 추가
	auto& SceneManager = Engine->GetSceneManager();
	auto& Renderer = Engine->GetRenderer();

	SceneManager.Update(dt);
	Renderer.Prepare();
	SceneManager.Render();
#ifdef _DEBUG
	EditorUpdate(dt);
#endif
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
	DrawSceneObjectsPanel();

	// 임시
	DrawCurrentScenePanel();

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

void UGameApp::DrawSceneObjectsPanel()
{
	if (!ImGui::Begin("Scene Objects"))
	{
		ImGui::End();
		return;
	}

	USceneManager* SceneManager = &Engine->GetSceneManager();
	UScene* CurrentScene = SceneManager->GetCurrentScene();

	if (CurrentScene == nullptr)
	{
		ImGui::TextUnformatted("No active scene.");
		ImGui::End();
		return;
	}

	const auto& GameObjects = CurrentScene->GetGameObjects();

	ImGui::Text("Current Scene: %s", SceneManager->GetCurrentSceneName().c_str());
	ImGui::Text("Game Object Count: %d", static_cast<int>(GameObjects.size()));
	ImGui::Separator();

	if (GameObjects.empty())
	{
		ImGui::TextUnformatted("Scene has no game objects.");
		ImGui::End();
		return;
	}

	for (int Index = 0; Index < static_cast<int>(GameObjects.size()); ++Index)
	{
		UGameObject* GameObject = GameObjects[Index];
		if (GameObject == nullptr)
		{
			continue;
		}

		ImGui::PushID(Index);

		const std::string ObjectLabel =
			std::string(GameObject->GetEditorTypeName()) + " [" + std::to_string(Index) + "]";

		if (ImGui::CollapsingHeader(ObjectLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			FVector3 Position = GameObject->GetPosition();
			float PositionValues[3] = { Position.x, Position.y, Position.z };
			if (ImGui::DragFloat3("Position", PositionValues, 0.01f))
			{
				GameObject->SetPosition(FVector3{ PositionValues[0], PositionValues[1], PositionValues[2] });
			}

			float Scale = GameObject->GetScale();
			if (ImGui::DragFloat("Scale", &Scale, 0.01f, 0.001f, 10.0f, "%.3f"))
			{
				GameObject->SetScale(Scale);
			}
		}

		ImGui::PopID();
	}

	ImGui::End();
}

// 임시
void UGameApp::DrawCurrentScenePanel()
{
	USceneManager* SceneManager = &Engine->GetSceneManager();
	UScene* CurrentScene = SceneManager->GetCurrentScene();

	if (CurrentScene)
	{
		CurrentScene->OnImGuiRender();
	}
}

