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
#include <algorithm>
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

	FVector ExtractLocaton(const FMatrix& matrix) {
		return FVector{ matrix[3][0], matrix[3][1], matrix[3][2] };
	}

	FVector ExtractScale(const FMatrix& matrix) {
		const float sx = std::sqrt(matrix[0][0] * matrix[0][0] + matrix[0][1] * matrix[0][1] + matrix[0][2] * matrix[0][2]);
		const float sy = std::sqrt(matrix[1][0] * matrix[1][0] + matrix[1][1] * matrix[1][1] + matrix[1][2] * matrix[1][2]);
		const float sz = std::sqrt(matrix[2][0] * matrix[2][0] + matrix[2][1] * matrix[2][1] + matrix[2][2] * matrix[2][2]);
		return FVector{sx, sy, sz };
	}

	FQuat ExtractRotation(const FMatrix& mat)
	{
		FVector scale = ExtractScale(mat);

		// scale 제거해서 순수 회전 행렬만 남김
		// r[행][열]
		float r[3][3] = {
			{ mat[0][0] / scale.X,  mat[0][1] / scale.Y,  mat[0][2] / scale.Z },
			{ mat[1][0] / scale.X,  mat[1][1] / scale.Y,  mat[1][2] / scale.Z },
			{ mat[2][0] / scale.X,  mat[2][1] / scale.Y,  mat[2][2] / scale.Z },
		};

		// 회전 행렬 → 쿼터니언
		float trace = r[0][0] + r[1][1] + r[2][2];
		FQuat q;

		if (trace > 0.0f) {
			float s = 0.5f / sqrtf(trace + 1.0f);
			q.W = 0.25f / s;
			q.X = (r[2][1] - r[1][2]) * s;
			q.Y = (r[0][2] - r[2][0]) * s;
			q.Z = (r[1][0] - r[0][1]) * s;
		}
		else if (r[0][0] > r[1][1] && r[0][0] > r[2][2]) {
			float s = 2.0f * sqrtf(1.0f + r[0][0] - r[1][1] - r[2][2]);
			q.W = (r[2][1] - r[1][2]) / s;
			q.X = 0.25f * s;
			q.Y = (r[0][1] + r[1][0]) / s;
			q.Z = (r[0][2] + r[2][0]) / s;
		}
		else if (r[1][1] > r[2][2]) {
			float s = 2.0f * sqrtf(1.0f + r[1][1] - r[0][0] - r[2][2]);
			q.W = (r[0][2] - r[2][0]) / s;
			q.X = (r[0][1] + r[1][0]) / s;
			q.Y = 0.25f * s;
			q.Z = (r[1][2] + r[2][1]) / s;
		}
		else {
			float s = 2.0f * sqrtf(1.0f + r[2][2] - r[0][0] - r[1][1]);
			q.W = (r[1][0] - r[0][1]) / s;
			q.X = (r[0][2] + r[2][0]) / s;
			q.Y = (r[1][2] + r[2][1]) / s;
			q.Z = 0.25f * s;
		}

		return q;
	}

	FRotator QuatToRotator(const FQuat& q)
	{
		const float RAD2DEG = 180.0f / 3.14159265f;

		// Pitch (X)
		float sinp = 2.0f * (q.W * q.Y - q.Z * q.X);
		float pitch = (fabsf(sinp) >= 1.0f)
			? copysignf(90.0f, sinp)
			: asinf(sinp) * RAD2DEG;

		// Yaw (Z)
		float yaw = atan2f(
			2.0f * (q.W * q.Z + q.X * q.Y),
			1.0f - 2.0f * (q.Y * q.Y + q.Z * q.Z)
		) * RAD2DEG;

		// Roll (Y)
		float roll = atan2f(
			2.0f * (q.W * q.X + q.Y * q.Z),
			1.0f - 2.0f * (q.X * q.X + q.Y * q.Y)
		) * RAD2DEG;

		return { roll, pitch, yaw };
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
	

	FRay MouseRay = FPickingSystem::BuildPickRay(*Camera, Input->GetMouseX(), Input->GetMouseY(), RHI->GetViewportWidth(), RHI->GetViewportHeight());

	if (Input->IsMouseButtonPressed(FInput::MOUSE_LEFT))
	{
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
			auto& ColdData = const_cast<FScenePrimitiveColdData&>(Scene->GetPrimitiveColdData()[PickState.SelectedPrimitiveIndex]);

			RuntimeData.SetWorldMatrix(*SelectedMatrixPtr);
			RuntimeData.SetRelativeLocation(ExtractLocaton(*SelectedMatrixPtr));
			//RuntimeData.SetRelativeLocation(ExtractScale(*SelectedMatrixPtr));
			RuntimeData.GetComponentToWorld();
			//void FSceneComponent::UpdateComponentToWorld() const
	
		}
	}
	else if (Input->IsMouseButtonReleased(FInput::MOUSE_LEFT))
	{
		Gizmo->EndDrag();
		SceneGraph->Build(*Scene);
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

	return SceneLoader->LoadScene(
		ScenePath.wstring(),
		this,
		Scene.get(),
		RHI.get(),
		Camera.get(),
		VisibilitySystem.get(),
		PickingSystem.get()
	);
}
