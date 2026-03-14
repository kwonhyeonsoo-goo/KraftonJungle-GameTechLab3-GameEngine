#include "EngineTest.h"
#include "Core/Core.h"
#include "Renderer/Renderer.h"
#include "Object/Scene/Scene.h"
#include "Object/Actor/Actor.h"
#include "Component/SceneComponent.h"
#include "Picking/Picker.h"
#include "Camera/Camera.h"
#include "Component/SphereComponent.h"
#include "Component/CubeComponent.h"
#include "Object/Class.h"

#include <filesystem>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

static CCore* GCore = nullptr;
static CPicker GPicker;
static AActor* GSelectedActor = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		if (GCore && GCore->GetRenderer() && wParam != SIZE_MINIMIZED)
		{
			GCore->GetRenderer()->OnResize(LOWORD(lParam), HIWORD(lParam));
		}
		return 0;
	case WM_LBUTTONDOWN:
		// ImGui가 마우스를 사용 중이면 피킹하지 않음
		if (!ImGui::GetIO().WantCaptureMouse && GCore && GCore->GetScene())
		{
			int MouseX = LOWORD(lParam);
			int MouseY = HIWORD(lParam);
			RECT Rect;
			GetClientRect(hWnd, &Rect);
			int Width = Rect.right - Rect.left;
			int Height = Rect.bottom - Rect.top;

			AActor* Picked = GPicker.PickActor(GCore->GetScene(), MouseX, MouseY, Width, Height);
			if (Picked)
			{
				GSelectedActor = Picked;
				GCore->SetSelectedActor(GSelectedActor);
			}
		}
		break;
	default:
		if (GCore)
		{
			GCore->ProcessInput(hWnd, msg, wParam, lParam);
		}
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void SetupImGuiCallbacks(CRenderer* Renderer)
{
	HWND Hwnd = Renderer->GetHwnd();
	ID3D11Device* Device = Renderer->GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer->GetDeviceContext();

	Renderer->SetGUICallbacks(
		// Init
		[Hwnd, Device, DeviceContext]()
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

			ImGui::StyleColorsDark();

			ImGuiStyle& style = ImGui::GetStyle();
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				style.WindowRounding = 0.0f;
				style.Colors[ImGuiCol_WindowBg].w = 1.0f;
			}

			ImGui_ImplWin32_Init(Hwnd);
			ImGui_ImplDX11_Init(Device, DeviceContext);
		},
		// Shutdown
		[]()
		{
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		},
		// NewFrame
		[]()
		{
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
		},
		// Render
		[]()
		{
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		},
		// PostPresent
		[]()
		{
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		}
	);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	// Window
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"TestWindow";
	RegisterClassEx(&wc);

	// 원하는 클라이언트 크기
	int desiredWidth = 1280;
	int desiredHeight = 720;

	// 기본 창 스타일
	DWORD dwStyle = WS_OVERLAPPEDWINDOW;

	// 확장 스타일 (필요하다면 변경 가능)
	DWORD dwExStyle = 0;

	// RECT 구조체에 원하는 클라이언트 크기 설정
	RECT rect = { 0, 0, desiredWidth, desiredHeight };

	// 창 크기를 계산
	AdjustWindowRectEx(&rect, dwStyle, TRUE, dwExStyle);
	int windowWidth = rect.right - rect.left;
	int windowHeight = rect.bottom - rect.top;

	HWND hwnd = CreateWindowEx(
		dwExStyle,
		L"TestWindow",
		L"Renderer Test",
		dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight,
		nullptr, nullptr, hInstance, nullptr
	);
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	RECT clientRect = {};
	GetClientRect(hwnd, &clientRect);
	int clientWidth = clientRect.right - clientRect.left;
	int clientHeight = clientRect.bottom - clientRect.top;

	// Core
	CCore Core;
	GCore = &Core;
	if (!Core.Initialize(hwnd, clientWidth, clientHeight))
		return -1;

	// ImGui
	SetupImGuiCallbacks(Core.GetRenderer());

	// ImGui test widgets
	bool show_demo_window = true;
	bool show_another_window = false;
	Core.GetRenderer()->SetGUIUpdateCallback([&]()
	{
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");
			ImGui::Text("This is some useful text.");
			ImGui::Checkbox("Demo Window", &show_demo_window);
			ImGui::Checkbox("Another Window", &show_another_window);
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);

			if (ImGui::Button("Button"))
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGuiIO& io = ImGui::GetIO();
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		// Jungle Control Panel
		ImGui::Begin("Jungle Control Panel");
		if (GCore && GCore->GetScene())
		{
			CCamera* Cam = GCore->GetScene()->GetCamera();
			if (Cam)
			{
				ImGui::SeparatorText("Camera");

				FVector CamPos = Cam->GetPosition();
				float Pos[3] = { CamPos.X, CamPos.Y, CamPos.Z };
				if (ImGui::DragFloat3("Position", Pos, 0.1f))
				{
					Cam->SetPosition({ Pos[0], Pos[1], Pos[2] });
				}

				float CamYaw = Cam->GetYaw();
				float CamPitch = Cam->GetPitch();
				bool RotChanged = false;
				RotChanged |= ImGui::DragFloat("Yaw", &CamYaw, 0.5f);
				RotChanged |= ImGui::DragFloat("Pitch", &CamPitch, 0.5f, -89.0f, 89.0f);
				if (RotChanged)
				{
					Cam->SetRotation(CamYaw, CamPitch);
				}

				float CamFOV = Cam->GetFOV();
				if (ImGui::SliderFloat("FOV", &CamFOV, 10.0f, 120.0f))
				{
					Cam->SetFOV(CamFOV);
				}
			}

			ImGui::SeparatorText("Spawn");

			static int SpawnTypeIndex = 0;
			const char* SpawnTypes[] = { "Cube", "Sphere" };
			ImGui::Combo("Type", &SpawnTypeIndex, SpawnTypes, IM_ARRAYSIZE(SpawnTypes));

			if (ImGui::Button("Spawn"))
			{
				UScene* Scene = GCore->GetScene();
				static int SpawnCount = 0;
				FString Name = FString(SpawnTypes[SpawnTypeIndex]) + "_Spawned_" + std::to_string(SpawnCount++);
				AActor* NewActor = Scene->SpawnActor<AActor>(Name);

				UActorComponent* Comp = nullptr;
				if (SpawnTypeIndex == 0)
					Comp = new UCubeComponent();
				else
					Comp = new USphereComponent();

				NewActor->AddOwnedComponent(Comp);

				GSelectedActor = NewActor;
				GCore->SetSelectedActor(GSelectedActor);
			}

			ImGui::SeparatorText("Scene");

			static char SceneName[128] = "NewScene";
			ImGui::InputText("Scene Name", SceneName, IM_ARRAYSIZE(SceneName));

			if (ImGui::Button("Save"))
			{
				FString Path = FString("../Assets/Scenes/") + SceneName + ".json";
				GCore->GetScene()->SaveSceneToFile(Path);
			}

			ImGui::Spacing();

			// 저장된 Scene 목록
			static std::vector<std::string> SceneFiles;
			static int SelectedSceneIndex = -1;

			if (ImGui::Button("Refresh List"))
			{
				SceneFiles.clear();
				SelectedSceneIndex = -1;
				const std::string ScenesDir = "../Assets/Scenes";
				if (std::filesystem::exists(ScenesDir))
				{
					for (auto& Entry : std::filesystem::directory_iterator(ScenesDir))
					{
						if (Entry.path().extension() == ".json")
						{
							SceneFiles.push_back(Entry.path().stem().string());
						}
					}
				}
			}

			if (!SceneFiles.empty())
			{
				if (ImGui::BeginListBox("Scenes"))
				{
					for (int i = 0; i < static_cast<int>(SceneFiles.size()); ++i)
					{
						bool bSelected = (SelectedSceneIndex == i);
						if (ImGui::Selectable(SceneFiles[i].c_str(), bSelected))
						{
							SelectedSceneIndex = i;
						}
					}
					ImGui::EndListBox();
				}

				if (SelectedSceneIndex >= 0 && ImGui::Button("Load"))
				{
					GSelectedActor = nullptr;
					GCore->SetSelectedActor(nullptr);
					GCore->GetScene()->ClearActors();

					FString Path = FString("../Assets/Scenes/") + SceneFiles[SelectedSceneIndex] + ".json";
					GCore->GetScene()->LoadSceneFromFile(Path);
				}
			}
		}
		ImGui::End();

		// 선택된 Actor 정보 표시
		ImGui::Begin("Property");
		if (GSelectedActor)
		{
			ImGui::Text("Selected: %s", GSelectedActor->GetName().c_str());

			USceneComponent* Root = GSelectedActor->GetRootComponent();
			if (Root)
			{
				FTransform Transform = Root->GetRelativeTransform();

				float Loc[3] = { Transform.Location.X, Transform.Location.Y, Transform.Location.Z };
				float Rot[3] = { Transform.Rotation.X, Transform.Rotation.Y, Transform.Rotation.Z };
				float Scl[3] = { Transform.Scale.X, Transform.Scale.Y, Transform.Scale.Z };

				if (ImGui::DragFloat3("Location", Loc, 0.1f))
				{
					Transform.Location = { Loc[0], Loc[1], Loc[2] };
					Root->SetRelativeTransform(Transform);
				}
				if (ImGui::DragFloat3("Rotation", Rot, 0.1f))
				{
					Transform.Rotation = { Rot[0], Rot[1], Rot[2] };
					Root->SetRelativeTransform(Transform);
				}
				if (ImGui::DragFloat3("Scale", Scl, 0.1f))
				{
					Transform.Scale = { Scl[0], Scl[1], Scl[2] };
					Root->SetRelativeTransform(Transform);
				}
			}
		}
		else
		{
			ImGui::Text("No actor selected");
		}
		ImGui::End();
	});

	// Timing
	LARGE_INTEGER Frequency, LastTime, CurrentTime;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&LastTime);

	// Main loop
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
			if (Core.GetRenderer()->IsOccluded())
				continue;

			QueryPerformanceCounter(&CurrentTime);
			float DeltaTime = static_cast<float>(CurrentTime.QuadPart - LastTime.QuadPart)
				/ static_cast<float>(Frequency.QuadPart);
			LastTime = CurrentTime;

			Core.Tick(DeltaTime);
		}
	}

	// Cleanup
	GCore = nullptr;
	Core.Release();

	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}
