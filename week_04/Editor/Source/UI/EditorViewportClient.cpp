#include "EditorViewportClient.h"

#include "EditorUI.h"
#include "Actor/Actor.h"
#include "Core/Core.h"
#include "Input/InputManager.h"
#include "Debug/EngineLog.h"
#include "Platform/Windows/Window.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderStateManager.h"
#include "Renderer/Materialmanager.h"
#include "Renderer/Material.h"
#include "Renderer/ShaderMap.h"
#include "World/Level.h"
#include "Serializer/SceneSerializer.h"
#include "Component/PrimitiveComponent.h"
#include "Core/Paths.h"
#include "imgui.h"
#include "Actor/ObjActor.h"
#include "Actor/SkySphereActor.h"
#include "Actor/StaticMeshActor.h"
#include "Asset/AssetManager.h"

FEditorViewportClient::FEditorViewportClient(FEditorUI& InEditorUI, FWindow* InMainWindow)
	: EditorUI(InEditorUI)
	, MainWindow(InMainWindow)
{
}

void FEditorViewportClient::Attach(FCore* Core, FRenderer* Renderer)
{
	if (!Core || !Renderer || !MainWindow) return;

	EditorUI.Initialize(Core);
	EditorUI.SetupWindow(MainWindow);
	EditorUI.AttachToRenderer(Renderer); // bViewportActive 가드로 중복 방지

	WireFrameMaterial = FMaterialManager::Get().FindByName(WireframeMaterialName);
	CreateGridResource(Renderer);
}

void FEditorViewportClient::CreateGridResource(FRenderer* Renderer)
{
	ID3D11Device* Device = Renderer->GetDevice();
	if (!Device) return;

	GridMesh = std::make_unique<FMeshData>();
	GridMesh->Topology = EMeshTopology::EMT_TriangleList;
	for (int i = 0; i < 18; ++i)
	{
		FPrimitiveVertex v;
		GridMesh->Vertices.push_back(v);
		GridMesh->Indices.push_back(i);
	}
	GridMesh->CreateVertexAndIndexBuffer(Device);

	std::wstring ShaderDirW = FPaths::ShaderDir();
	auto VS = FShaderMap::Get().GetOrCreateVertexShader(Device, (ShaderDirW + L"AxisVertexShader.hlsl").c_str());
	auto PS = FShaderMap::Get().GetOrCreatePixelShader(Device, (ShaderDirW + L"AxisPixelShader.hlsl").c_str());

	GridMaterial = std::make_shared<FMaterial>();
	GridMaterial->SetOriginName("M_EditorGrid");
	GridMaterial->SetVertexShader(VS);
	GridMaterial->SetPixelShader(PS);

	FRasterizerStateOption RastOpt;
	RastOpt.FillMode = D3D11_FILL_SOLID;
	RastOpt.CullMode = D3D11_CULL_NONE;
	GridMaterial->SetRasterizerOption(RastOpt);
	GridMaterial->SetRasterizerState(Renderer->GetRenderStateManager()->GetOrCreateRasterizerState(RastOpt));

	FDepthStencilStateOption DSSOpt;
	DSSOpt.DepthEnable = true;
	DSSOpt.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	GridMaterial->SetDepthStencilOption(DSSOpt);
	GridMaterial->SetDepthStencilState(Renderer->GetRenderStateManager()->GetOrCreateDepthStencilState(DSSOpt));

	int32 SlotIndex = GridMaterial->CreateConstantBuffer(Device, 32);
	if (SlotIndex >= 0)
	{
		GridMaterial->RegisterParameter("GridSize", SlotIndex, 12, 4);
		GridMaterial->RegisterParameter("LineThickness", SlotIndex, 16, 4);
		GridMaterial->SetParameterData("GridSize", &GridSize, 4);
		GridMaterial->SetParameterData("LineThickness", &LineThickness, 4);
	}
}

void FEditorViewportClient::Detach(FCore* Core, FRenderer* Renderer)
{
	Gizmo.EndDrag();
	EditorUI.DetachFromRenderer(Renderer);
	GridMesh.reset();
	GridMaterial.reset();
}

void FEditorViewportClient::Tick(FCore* Core, float DeltaTime)
{
	if (!Core) return;
	if (ImGui::GetCurrentContext())
	{
		const ImGuiIO& IO = ImGui::GetIO();
		if ((IO.WantCaptureKeyboard || IO.WantCaptureMouse) && !EditorUI.IsViewportInteractive())
			return;
	}
	if (!EditorUI.IsViewportInteractive()) return;
	IViewportClient::Tick(Core, DeltaTime);
}

void FEditorViewportClient::HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (!Core || !EditorUI.IsViewportInteractive()) return;
	if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse && !EditorUI.IsViewportInteractive()) return;

	ULevel* Level = ResolveLevel(Core);
	AActor* SelectedActor = Core->GetSelectedActor();
	if (!Level) return;

	// ── 윈도우 마우스 좌표 ──────────────────────────────────────────────
	const int32 WindowMouseX = static_cast<int32>(static_cast<short>(LOWORD(LParam)));
	const int32 WindowMouseY = static_cast<int32>(static_cast<short>(HIWORD(LParam)));

	// ── 호버된 뷰포트 인덱스로 로컬 좌표 획득 ───────────────────────────
	const int32 HoveredIndex = EditorUI.GetHoveredViewportIndex();
	const bool bHasViewportMouse = (HoveredIndex >= 0) &&
		EditorUI.GetMousePositionInViewport(
			HoveredIndex,
			WindowMouseX, WindowMouseY,
			ScreenMouseX, ScreenMouseY,
			ScreenWidth, ScreenHeight);

	// ── 해당 뷰포트의 카메라 ViewInfo ───────────────────────────────────
	// 이 ViewportClient 자신이 해당 뷰포트면 GetCameraViewInfo() 사용
	// 아니면 EditorUI의 해당 뷰포트 ViewportClient에서 가져옴
	FCameraViewInfo ActiveViewInfo;
	if (HoveredIndex >= 0)
	{
		if (IViewportClient* HoveredVP = EditorUI.GetViewportClientAt(HoveredIndex))
		{
			ActiveViewInfo = HoveredVP->GetCameraViewInfo();
		}
	}
	else
	{
		ActiveViewInfo = GetCameraViewInfo();
	}

	const bool bRightMouseDown = Core->GetInputManager() &&
		Core->GetInputManager()->IsMouseButtonDown(FInputManager::MOUSE_RIGHT);

	switch (Msg)
	{
	case WM_KEYDOWN:
		if (bRightMouseDown) return;
		switch (WParam)
		{
		case 'W': Gizmo.SetMode(EGizmoMode::Location); return;
		case 'E': Gizmo.SetMode(EGizmoMode::Rotation); return;
		case 'R': Gizmo.SetMode(EGizmoMode::Scale);    return;
		case 'L':
			Gizmo.ToggleCoordinateSpace();
			UE_LOG("Gizmo Space: %s", Gizmo.GetCoordinateSpace() == EGizmoCoordinateSpace::Local ? "Local" : "World");
			return;
		default: return;
		}

	case WM_LBUTTONDOWN:
		if (!bHasViewportMouse) return;

		if (SelectedActor && Gizmo.BeginDrag(SelectedActor, Level,
			ActiveViewInfo, Picker,
			ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight))
		{
			return;
		}
		{
			// 호버된 뷰포트 카메라 기준으로 피킹
			AActor* Picked = Picker.PickActor(Level, ActiveViewInfo,
				ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight);
			Core->SetSelectedActor(Picked);
			EditorUI.SyncSelectedActorProperty();
		}
		return;

	case WM_MOUSEMOVE:
		if (!bHasViewportMouse) { Gizmo.ClearHover(); return; }
		if (!Gizmo.IsDragging())
		{
			Gizmo.UpdateHover(SelectedActor, Level, ActiveViewInfo, Picker,
				ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight);
			return;
		}
		if (Gizmo.UpdateDrag(SelectedActor, Level, ActiveViewInfo, Picker,
			ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight))
		{
			EditorUI.SyncSelectedActorProperty();
		}
		return;

	case WM_LBUTTONUP:
		if (Gizmo.IsDragging())
		{
			Gizmo.EndDrag();
			if (bHasViewportMouse)
				Gizmo.UpdateHover(SelectedActor, Level, ActiveViewInfo, Picker,
					ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight);
			else
				Gizmo.ClearHover();
			EditorUI.SyncSelectedActorProperty();
		}
		return;

	default: return;
	}
}

void FEditorViewportClient::HandleFileDoubleClick(const FString& FilePath)
{
	FCore* Core = EditorUI.GetCore();
	if (!Core) return;

	if (FilePath.ends_with(".json"))
	{
		Core->SetSelectedActor(nullptr);
		Core->GetLevel()->ClearActors();

		bool bLoaded = FSceneSerializer::Load(
			Core->GetLevel(), FilePath,
			Core->GetRenderer()->GetDevice(),
			GetActiveCamera());

		if (bLoaded) UE_LOG("Level loaded: %s", FilePath.c_str());
		else MessageBoxW(nullptr, L"Level 정보가 잘못되었습니다.", L"Error", MB_OK | MB_ICONWARNING);
	}
}

void FEditorViewportClient::HandleFileDropOnViewport(const FString& FilePath)
{
	FCore* Core = EditorUI.GetCore();

	if (Core && Core->GetRenderer())
	{
		if (FilePath.ends_with(".obj"))
		{
			const FRay Ray = Picker.ScreenToRay(GetCameraViewInfo(), ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight);


			AStaticMeshActor* NewActor = Core->GetLevel()->SpawnActor<AStaticMeshActor>("StaticMeshActor");
			UStaticMesh* StaticMesh = FAssetManager::LoadObjStaticMesh(FPaths::ToRelativePath(FilePath));
			NewActor->SetStaticMesh(StaticMesh);

			FVector V = Ray.Origin + Ray.Direction * 5;
			NewActor->SetActorLocation(V);

		}
	}
}

void FEditorViewportClient::BuildRenderCommands(FCore* Core, ULevel* Level,
	const FFrustum& Frustum, FRenderCommandQueue& OutQueue)
{
	IViewportClient::BuildRenderCommands(Core, Level, Frustum, OutQueue);

	if (RenderMode == ERenderMode::Wireframe)
	{
		for (auto& Cmd : OutQueue.Commands)
		{
			if (Cmd.RenderLayer != ERenderLayer::Overlay)
				Cmd.Material = WireFrameMaterial.get();
		}
	}

	if (!Core || !Level) return;

	if (GridMesh && GridMaterial && bShowGrid)
	{
		FRenderCommand GridCmd;
		GridCmd.MeshData = GridMesh.get();
		GridCmd.Material = GridMaterial.get();
		GridCmd.WorldMatrix = FMatrix::Identity;
		GridCmd.RenderLayer = ERenderLayer::Default;
		OutQueue.AddCommand(GridCmd);
	}

	AActor* GizmoTarget = Core->GetSelectedActor();
	if (GizmoTarget && !GizmoTarget->IsA<ASkySphereActor>())
	{
		Gizmo.BuildRenderCommands(GizmoTarget, GetCameraViewInfo(), OutQueue);
	}
}

void FEditorViewportClient::SetGridSize(float InSize)
{
	GridSize = InSize;
	if (GridMaterial) GridMaterial->SetParameterData("GridSize", &GridSize, 4);
}

void FEditorViewportClient::SetLineThickness(float InThickness)
{
	LineThickness = InThickness;
	if (GridMaterial) GridMaterial->SetParameterData("LineThickness", &LineThickness, 4);
}