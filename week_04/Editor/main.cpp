#include "EngineTest.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"
#include "Camera/Camera.h"
#include "Component/SphereComponent.h"
#include "Component/CubeComponent.h"
#include "Primitive/PrimitiveBase.h"
#include <iostream>

// ─── 입력 상태 ───
static bool bRightMouseDown = false;
static POINT LastMousePos = {};

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_RBUTTONDOWN:
		bRightMouseDown = true;
		GetCursorPos(&LastMousePos);
		SetCapture(hWnd);
		return 0;
	case WM_RBUTTONUP:
		bRightMouseDown = false;
		ReleaseCapture();
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	// ─── 윈도우 생성 ───
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"TestWindow";
	RegisterClassEx(&wc);

	HWND hwnd = CreateWindowEx(
		0, L"TestWindow", L"Renderer Test",
		WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720,
		nullptr, nullptr, hInstance, nullptr
	);
	ShowWindow(hwnd, SW_SHOW);

	// ─── Renderer 초기화 ───
	CRenderer renderer;
	if (!renderer.Initialize(hwnd, 1280, 720))
		return -1;

	ID3D11Device* device = renderer.GetDevice();
	ID3D11DeviceContext* context = renderer.GetDeviceContext();

	// ─── 셰이더 로드 ───
	CShaderManager shader;
	if (!shader.LoadVertexShader(device, L"../Engine/Shaders/VertexShader.hlsl"))
	{
		MessageBox(0, L"VertexShader load failed", 0, 0);
		return -1;
	}
	if (!shader.LoadPixelShader(device, L"../Engine/Shaders/PixelShader.hlsl"))
	{
		MessageBox(0, L"PixelShader load failed", 0, 0);
		return -1;
	}

	// ─── 카메라 ───
	CCamera camera;
	camera.SetPosition({ 0.0f, 2.0f, -5.0f });
	camera.SetRotation(0.0f, -15.0f);
	camera.SetAspectRatio(1280.0f / 720.0f);

	// ─── 오브젝트 배치 ───
	USphereComponent sphere;
	sphere.SetRelativeLocation({ 0.0f, 0.0f, 0.0f });

	UCubeComponent cube;
	cube.SetRelativeLocation({ 3.0f, 0.0f, 0.0f });

	// ─── Rasterizer State ───
	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_BACK;
	ID3D11RasterizerState* rsState = nullptr;
	device->CreateRasterizerState(&rsDesc, &rsState);

	// ─── 타이밍 ───
	LARGE_INTEGER Frequency, LastTime, CurrentTime;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&LastTime);

	// ─── 메인 루프 ───
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// 델타 타임
			QueryPerformanceCounter(&CurrentTime);
			float DeltaTime = static_cast<float>(CurrentTime.QuadPart - LastTime.QuadPart)
				/ static_cast<float>(Frequency.QuadPart);
			LastTime = CurrentTime;

			// ─── 키보드 입력 (WASD + QE) ───
			if (GetAsyncKeyState('W') & 0x8000) camera.MoveForward(DeltaTime);
			if (GetAsyncKeyState('S') & 0x8000) camera.MoveForward(-DeltaTime);
			if (GetAsyncKeyState('D') & 0x8000) camera.MoveRight(DeltaTime);
			if (GetAsyncKeyState('A') & 0x8000) camera.MoveRight(-DeltaTime);
			if (GetAsyncKeyState('E') & 0x8000) camera.MoveUp(DeltaTime);
			if (GetAsyncKeyState('Q') & 0x8000) camera.MoveUp(-DeltaTime);

			// ─── 마우스 우클릭 드래그 → 카메라 회전 ───
			if (bRightMouseDown)
			{
				POINT CurrentMousePos;
				GetCursorPos(&CurrentMousePos);
				float DeltaX = static_cast<float>(CurrentMousePos.x - LastMousePos.x);
				float DeltaY = static_cast<float>(CurrentMousePos.y - LastMousePos.y);
				LastMousePos = CurrentMousePos;

				camera.Rotate(DeltaX * 0.2f, -DeltaY * 0.2f);
			}

			// ─── 렌더링 ───
			renderer.BeginFrame();

			shader.Bind(context);
			context->RSSetState(rsState);

			FMatrix VP = camera.GetViewMatrix() * camera.GetProjectionMatrix();
			renderer.ViewProjectionMatrix = VP;

			FMeshData* sphereMesh = sphere.GetPrimitive()->GetMeshData();
			renderer.AddCommand({ sphereMesh, sphere.RelativeTransform.ToMatrix() });

			//FMeshData* cubeMesh = cube.GetPrimitive()->GetMeshData();
			//renderer.AddCommand({ cubeMesh, cube.RelativeTransform.ToMatrix() });

			renderer.ExecuteCommands();

			renderer.EndFrame();
		}
	}

	// ─── 정리 ───
	if (rsState) rsState->Release();
	shader.Release();
	CPrimitiveBase::ClearCache();
	renderer.Release();

	return 0;
}
