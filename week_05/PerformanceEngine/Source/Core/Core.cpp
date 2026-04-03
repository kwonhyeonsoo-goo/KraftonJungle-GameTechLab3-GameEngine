#include "Core.h"

#include <array>
#include <filesystem>

#include "Camera/Camera.h"
#include "Graphics/D3D11/D3D11RHI.h"
#include "Grid/Grid.h"
#include "Hud/HudRenderer.h"
#include "Input/Input.h"
#include "Renderer/SceneRenderer.h"
#include "Scene/Scene.h"
#include "Stats/StatsSystem.h"
#include "Visibility/VisibilitySystem.h"

namespace
{
	constexpr float DefaultCameraSpeed = 20.0f;
	constexpr float DefaultCameraSensitivity = 0.12f;

	std::filesystem::path SearchForSceneFrom(const std::filesystem::path& InStartDirectory)
	{
		static const std::array<std::filesystem::path, 2> RelativeCandidates =
		{
			std::filesystem::path(L"PerformanceEngine/Data/Scene/Default.scene"),
			std::filesystem::path(L"Data/Scene/Default.scene"),
		};

		std::filesystem::path Cursor = InStartDirectory;
		while (!Cursor.empty())
		{
			for (const std::filesystem::path& RelativeCandidate : RelativeCandidates)
			{
				const std::filesystem::path Candidate = Cursor / RelativeCandidate;
				if (std::filesystem::exists(Candidate))
				{
					return std::filesystem::absolute(Candidate);
				}
			}

			if (!Cursor.has_parent_path())
			{
				break;
			}

			const std::filesystem::path Parent = Cursor.parent_path();
			if (Parent == Cursor)
			{
				break;
			}

			Cursor = Parent;
		}

		return {};
	}

	std::filesystem::path FindDefaultScenePath()
	{
		if (const std::filesystem::path CurrentCandidate = SearchForSceneFrom(std::filesystem::current_path()); !CurrentCandidate.empty())
		{
			return CurrentCandidate;
		}

		std::array<wchar_t, MAX_PATH> ModulePath = {};
		const DWORD CharacterCount = GetModuleFileNameW(nullptr, ModulePath.data(), static_cast<DWORD>(ModulePath.size()));
		if (CharacterCount > 0)
		{
			const std::filesystem::path ModuleDirectory = std::filesystem::path(ModulePath.data()).parent_path();
			return SearchForSceneFrom(ModuleDirectory);
		}

		return {};
	}
}

FCore::FCore() = default;
FCore::~FCore() = default;

bool FCore::Initialize(const FCoreInitArgs& Args)
{
	if (Args.Hwnd == nullptr)
	{
		return false;
	}

	Input = std::make_unique<FInput>();
	Camera = std::make_unique<FCamera>();
	RHI = std::make_unique<FD3D11RHI>();
	Scene = std::make_unique<FScene>();
	SceneRenderer = std::make_unique<FSceneRenderer>();
	HudRenderer = std::make_unique<FHudRenderer>();
	VisibilitySystem = std::make_unique<FVisibilitySystem>();
	PickingSystem = std::make_unique<FPickingSystem>();
	StatsSystem = std::make_unique<FStatsSystem>();

	if (!Input || !Camera || !RHI || !Scene || !SceneRenderer || !HudRenderer || !VisibilitySystem || !PickingSystem || !StatsSystem)
	{
		Release();
		return false;
	}

	if (!RHI->Initialize(Args.Hwnd))
	{
		Release();
		return false;
	}

	Grid = std::make_unique<FGrid>();
	if (Grid && !Grid->Initialize(*RHI))
	{
		OutputDebugStringA("[Core] Failed to initialize grid renderer. Continuing without grid.\n");
		Grid.reset();
	}

	if (!LoadDefaultScene())
	{
		Release();
		return false;
	}

	const FSceneCameraInitData& InitialCamera = Scene->GetInitialCamera();
	Camera->SetTransform(InitialCamera.Transform);
	Camera->SetFOV(InitialCamera.FovDegrees);
	Camera->SetNearClip(InitialCamera.NearClip);
	Camera->SetFarClip(InitialCamera.FarClip);
	Camera->SetSpeed(DefaultCameraSpeed);
	Camera->SetSensitivity(DefaultCameraSensitivity);

	const int32 ViewportWidth = RHI->GetViewportWidth();
	const int32 ViewportHeight = RHI->GetViewportHeight();
	if (ViewportWidth > 0 && ViewportHeight > 0)
	{
		Camera->SetAspectRatio(static_cast<float>(ViewportWidth) / static_cast<float>(ViewportHeight));
	}
	else if (Args.Width > 0 && Args.Height > 0)
	{
		Camera->SetAspectRatio(static_cast<float>(Args.Width) / static_cast<float>(Args.Height));
	}

	if (!SceneRenderer->Initialize(*RHI) || !HudRenderer->Initialize(*RHI))
	{
		Release();
		return false;
	}

	VisibilitySystem->Reset();
	PickingSystem->Reset();
	StatsSystem->Reset();
	VisibilityResults = FVisibilityResults();
	PickState = FPickState();
	bInitialized = true;
	return true;
}

void FCore::Tick()
{
	StatsSystem->BeginFrame();
	Input->Tick();
	Camera->Update(*Input, static_cast<float>(StatsSystem->GetFrameTimeMs() * 0.001));

	VisibilitySystem->Build(*Scene, *Camera, VisibilityResults);

	if (Input->IsMouseButtonPressed(FInput::MOUSE_LEFT))
	{
		PickingSystem->UpdatePick(
			*Scene,
			*Camera,
			VisibilityResults,
			Input->GetMousePositionClient(),
			RHI->GetViewportWidth(),
			RHI->GetViewportHeight(),
			PickState);
	}

	StatsSystem->ApplyPickState(PickState);

	BeginFrame();
	SceneRenderer->Render(*RHI, *Scene, *Camera, VisibilityResults, PickState);

	if (Grid)
	{
		Grid->Render(*RHI, *Camera);
	}

	HudRenderer->Render(*RHI, *Camera, *Scene, *StatsSystem, PickState);
	EndFrame();

	StatsSystem->EndFrame();
}

void FCore::Shutdown()
{
	Release();
}

bool FCore::HandleMessage(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (Input)
	{
		Input->ProcessMessage(Hwnd, Msg, WParam, LParam);
	}

	return false;
}

void FCore::HandleResize(int32 Width, int32 Height)
{
	if (!RHI || Width <= 0 || Height <= 0)
	{
		return;
	}

	RHI->Resize(Width, Height);

	if (Camera)
	{
		Camera->SetAspectRatio(static_cast<float>(Width) / static_cast<float>(Height));
	}
}

void FCore::Release()
{
	if (Grid)
	{
		Grid->Release();
		Grid.reset();
	}

	if (HudRenderer)
	{
		HudRenderer->Shutdown();
		HudRenderer.reset();
	}

	if (SceneRenderer)
	{
		SceneRenderer->Shutdown();
		SceneRenderer.reset();
	}

	if (Scene)
	{
		Scene->Release();
		Scene.reset();
	}

	if (RHI)
	{
		RHI->Shutdown();
		RHI.reset();
	}

	StatsSystem.reset();
	PickingSystem.reset();
	VisibilitySystem.reset();
	Camera.reset();
	Input.reset();

	VisibilityResults = FVisibilityResults();
	PickState = FPickState();
	bInitialized = false;
}

void FCore::BeginFrame()
{
	if (RHI)
	{
		RHI->BeginFrame();
	}
}

void FCore::EndFrame()
{
	if (RHI)
	{
		RHI->EndFrame();
	}
}

bool FCore::LoadDefaultScene()
{
	if (!Scene || !RHI)
	{
		return false;
	}

	const std::filesystem::path ScenePath = FindDefaultScenePath();
	if (ScenePath.empty())
	{
		return false;
	}

	return Scene->LoadFromFile(RHI->GetDevice(), RHI->GetDeviceContext(), ScenePath);
}
