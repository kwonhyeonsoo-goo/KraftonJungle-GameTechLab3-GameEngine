#include "UGameApp.h"

#include <chrono>
#include <utility>

#include "URenderer.h"
#include "Utility.h"

UGameApp::UGameApp(HINSTANCE hInst, FWindowDesc desc) : UWindow(hInst, std::move(desc)), Renderer(nullptr)
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
}

void UGameApp::Tick(float dt)
{
	// TODO : Update / Render 추가
	Renderer->Prepare();
	Renderer->SwapBuffer();
}

void UGameApp::Shutdown()
{
	// TODO : 리소스 해제
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
