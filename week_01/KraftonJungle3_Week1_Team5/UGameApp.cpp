#include "UGameApp.h"

#include <chrono>
#include <utility>

#include "UBall.h"
#include "URenderer.h"
#include "Utility.h"
#include "UWeek0Scene.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

UGameApp::UGameApp(HINSTANCE hInst, FWindowDesc desc) : UWindow(hInst, std::move(desc)), Renderer(nullptr),
                                                        CurrentScene(nullptr)
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
	CreateRenderer();

	// ImGui 초기화
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)Handle());
	ImGui_ImplDX11_Init(Renderer->GetDevice(), Renderer->GetDeviceContext());

	CurrentScene = new UWeek0Scene();
	CurrentScene->Initialize(Renderer->GetDevice(), Renderer->GetDeviceContext());
}

void UGameApp::Tick(float dt)
{
	// TODO : Update / Render 추가
	Renderer->Prepare();
	CurrentScene->Render(Renderer->GetDevice(), Renderer->GetDeviceContext());

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 이후 ImGui UI 컨트롤 추가는 ImGui::NewFrame()과 ImGui::Render()
	// 사이인 이곳에 위치
	ImGui::Begin("Jungle Property Window");
	ImGui::Text("Hello Jungle World!");

	ImGui::End();

	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	Renderer->SwapBuffer();
}

void UGameApp::Shutdown()
{
	// TODO : 리소스 해제
	SafeReleaseAndDelete(CurrentScene);

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	SafeDelete(Renderer);
}

void UGameApp::OnDestroy()
{
	UWindow::OnDestroy();
}

LRESULT UGameApp::OnMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	return UWindow::OnMessage(msg, wp, lp);
}

void UGameApp::CreateRenderer()
{
	Renderer = new URenderer(Handle());
}
