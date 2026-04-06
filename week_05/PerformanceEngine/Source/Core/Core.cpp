#include "Core.h"

#include <array>
#include <filesystem>
#include <cmath>
#include "Gizmo/Gizmo.h"
#include "Camera/Camera.h"
#include "Graphics/D3D11/D3D11RHI.h"
#include "Grid/Grid.h"
#include "Hud/HudRenderer.h"
#include "Input/Input.h"
#include "Renderer/SceneRenderer.h"
#include "Scene/Scene.h"
#include "Stats/StatsSystem.h"
#include "Visibility/VisibilitySystem.h"
#include "FileSystem/FileSystem.h"
#include "Scene/SceneGraph.h"
#include "StaticMesh/StaticMesh.h" 
#include "Editor/EditorUI.h"
#include "Thirdparty/ImGui/imgui.h"
#include "Scene/SceneLoader.h"
#include "Renderer/ImpostorBaker.h"
#include <algorithm>
#include <unordered_set>
namespace
{
	constexpr float DefaultCameraSpeed = 20.0f;
	constexpr float DefaultCameraSensitivity = 0.12f;

	/*std::filesystem::path SearchForSceneFrom(const std::filesystem::path& InStartDirectory)
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
	}*/
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
	SceneGraph = std::make_unique<FSceneGraph>();
	SceneLoader = std::make_unique<FSceneLoader>();
	Gizmo = std::make_unique<FGizmo>();
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

	EditorUI = std::make_unique<FEditorUI>(this);
	EditorUI->Initialize(Args.Hwnd, RHI->GetDevice(), RHI->GetDeviceContext());

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

	if (!LoadDefaultScene())
	{
		Release();
		return false;
	}

	return true;
}

void FCore::Tick()
{
	StatsSystem->BeginFrame();
	Input->Tick();
	Camera->Update(*Input, static_cast<float>(StatsSystem->GetFrameTimeMs() * 0.001));

	//1.Frumstum culling
	VisibilitySystem->Build(*Scene, *Camera, VisibilityResults);
	FMatrix* SelectedMatrixPtr = nullptr;
	if (PickState.bHit && PickState.SelectedPrimitiveIndex >= 0 && PickState.SelectedPrimitiveIndex < Scene->GetPrimitiveRuntimeData().size())
	{
		SelectedMatrixPtr = const_cast<FMatrix*>(&Scene->GetPrimitiveRuntimeData()[PickState.SelectedPrimitiveIndex].GetComponentToWorld());
	}
	
	ImGuiIO& io = ImGui::GetIO();
	FRay MouseRay = FPickingSystem::BuildPickRay(*Camera, Input->GetMouseX(), Input->GetMouseY(), RHI->GetViewportWidth(), RHI->GetViewportHeight());

	if (Input->IsMouseButtonPressed(FInput::MOUSE_LEFT))
	{
		SceneGraph->Build(*Scene);
		PickingSystem->UpdatePick(
			*Scene,
			*Camera,
			VisibilityResults,
			Input->GetMousePositionClient(),
			RHI->GetViewportWidth(),
			RHI->GetViewportHeight(),
			*SceneGraph,
			Gizmo.get(),
			SelectedMatrixPtr, 
			PickState);
		StatsSystem->RecordPickEvent(PickState);
		if (PickState.bHitGizmo && SelectedMatrixPtr)
		{
			Gizmo->BeginDrag(SelectedMatrixPtr, Camera.get(), MouseRay, Input->GetMouseX(), Input->GetMouseY());
		}
		if (PickState.bHit)
		{
			SelectedPrimitiveData = Scene->GetPrimitiveRuntimeDataById(PickState.SelectedPrimitiveId);
		}
		else
		{
			SelectedPrimitiveData = nullptr;
		}
	}
	else if (Input->IsMouseButtonDown(FInput::MOUSE_LEFT))
	{
		if (Gizmo->IsDragging() && SelectedMatrixPtr)
		{
			Gizmo->UpdateDrag(SelectedMatrixPtr, Camera.get(), MouseRay, Input->GetMouseX(), Input->GetMouseY());

			auto& RuntimeData = const_cast<FScenePrimitiveRuntimeData&>(Scene->GetPrimitiveRuntimeData()[PickState.SelectedPrimitiveIndex]);

			RuntimeData.SetWorldMatrix(*SelectedMatrixPtr);

		}
	}
	else if (Input->IsMouseButtonReleased(FInput::MOUSE_LEFT))
	{
		Gizmo->EndDrag();
	}
	else
	{

		Gizmo->UpdateHover(SelectedMatrixPtr, Camera.get(), MouseRay);
	}

	StatsSystem->ApplyPickState(PickState);

	// 3. 기즈모 모드 변경 단축키 (W: 이동, E: 회전, R: 스케일, Q: 로컬/월드 토글)
	if (Input->IsKeyPressed('W')) Gizmo->SetMode(EGizmoMode::Location);
	if (Input->IsKeyPressed('E')) Gizmo->SetMode(EGizmoMode::Rotation);
	if (Input->IsKeyPressed('R')) Gizmo->SetMode(EGizmoMode::Scale);
	if (Input->IsKeyPressed('Q')) Gizmo->ToggleCoordinateSpace();
	StatsSystem->ApplyPickState(PickState);
	if (Input->IsKeyPressed('O'))
	{
		std::wstring SelectedPath;
		if (SceneLoader->OpenSceneFileDialog(SelectedPath))
		{
			SceneLoader->LoadScene(SelectedPath, this, Scene.get(), RHI.get(), Camera.get(), VisibilitySystem.get(), PickingSystem.get());
		}
	}
	const size_t PrimitiveCount = Scene->GetPrimitiveCount();
	RHI->EnsureCullingBufferCapacity(static_cast<uint32>(PrimitiveCount));
	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	if (SUCCEEDED(RHI->GetDeviceContext()->Map(RHI->InstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
	{
		FInstanceData* InstanceData = static_cast<FInstanceData*>(MappedResource.pData);
		const auto& PrimitiveRuntimeData = Scene->GetPrimitiveRuntimeData();

		for (size_t i = 0; i < PrimitiveCount; ++i)
		{
		
			InstanceData[i] = PrimitiveRuntimeData[i].CachedInstanceData;
		}

		RHI->GetDeviceContext()->Unmap(RHI->InstanceBuffer.Get(), 0);
	}

	const auto& PrimitiveRuntimeData = Scene->GetPrimitiveRuntimeData();
	std::sort(VisibilityResults.VisiblePrimitiveIndices.begin(), VisibilityResults.VisiblePrimitiveIndices.end(), [&](uint32 A, uint32 B) {
		const auto& DataA = PrimitiveRuntimeData[A];
		const auto& DataB = PrimitiveRuntimeData[B];
		if (!DataA.StaticMesh) return false;
		if (!DataB.StaticMesh) return true;
		return DataA.StaticMesh < DataB.StaticMesh;
	});
	BeginFrame();
	SceneRenderer->Render(*RHI, *Scene, *Camera, VisibilityResults, PickState);

	if (Grid)
	{
		Grid->Render(*RHI, *Camera);
	}
	if (Gizmo && PickState.bHit && SelectedMatrixPtr)
	{
		Gizmo->Render(*RHI, *Camera, *SelectedMatrixPtr);
	}
	HudRenderer->Render(*RHI, *Camera, *Scene, *StatsSystem, PickState);

	EditorUI->Render();

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
	if (EditorUI)
	{
		EditorUI->Shutdown();
		EditorUI.reset();
	}

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

	if (bInitialized && StatsSystem && RHI)
	{
		FBenchmarkRunMetadata Metadata;
		Metadata.AdapterName = RHI->GetAdapterName();
		Metadata.DedicatedVideoMemoryMB = RHI->GetAdapterDedicatedVideoMemoryMB();
		Metadata.ViewportWidth = RHI->GetViewportWidth();
		Metadata.ViewportHeight = RHI->GetViewportHeight();
		StatsSystem->WriteBenchmarkLogs(Metadata);
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
	if (!Scene || !RHI || !SceneLoader) return false;

	const std::filesystem::path ScenePath = FFileSystem::FindDefaultScenePath();
	if (ScenePath.empty()) return false;

	bool bLoaded = SceneLoader->LoadScene(
		ScenePath.wstring(),
		this,
		Scene.get(),
		RHI.get(),
		Camera.get(),
		VisibilitySystem.get(),
		PickingSystem.get()
	);

	// 씬의 모든 고유 메쉬에 대해 임포스터 베이킹
	if (bLoaded && Scene->GetPrimitiveCount() > 0)
	{
		OutputDebugStringA("\n[Core] Impostor baking start...\n");

		// 고유 메쉬 수집
		std::unordered_set<FStaticMesh*> UniqueMeshes;
		for (const auto& PrimData : Scene->GetPrimitiveRuntimeData())
		{
			if (PrimData.StaticMesh)
			{
				UniqueMeshes.insert(PrimData.StaticMesh);
			}
		}

		FImpostorBaker Baker;
		if (Baker.Initialize(RHI->GetDevice(), 2048, 16))
		{
			for (FStaticMesh* Mesh : UniqueMeshes)
			{
				// 메쉬 소스 경로에서 출력 파일명 생성
				// 예: "Data/apple_mid.obj" → "Data/Scene/apple_mid_Impostor"
				std::filesystem::path SrcPath = Mesh->GetSourcePath();
				std::wstring Stem = SrcPath.stem().wstring();
				std::wstring OutputPath = L"Data/Scene/" + Stem + L"_Impostor";

				// 항상 새로 베이크 (렌더 스테이트 수정 반영)
				Baker.Bake(RHI->GetDeviceContext(), Mesh, OutputPath);

				char msg[512];
				sprintf_s(msg, "[Core] Baked impostor for: %ls\n", Stem.c_str());
				OutputDebugStringA(msg);
			}
		}

		// 베이크 완료 후 SceneRenderer에 아틀라스 로드
		if (SceneRenderer)
		{
			SceneRenderer->LoadImpostorAtlases(*RHI, *Scene);
		}
	}

	return bLoaded;
}