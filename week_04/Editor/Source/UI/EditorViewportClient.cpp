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
FEditorViewportClient::FEditorViewportClient(FEditorUI& InEditorUI, FWindow* InMainWindow)
	: EditorUI(InEditorUI)
	, MainWindow(InMainWindow)
{
}

void FEditorViewportClient::Attach(FCore* Core, FRenderer* Renderer)
{
	if (!Core || !Renderer || !MainWindow)
	{
		return;
	}

	EditorUI.Initialize(Core);
	EditorUI.SetupWindow(MainWindow);
	EditorUI.AttachToRenderer(Renderer);

	// Wireframe 모드를 위한 머티리얼 가져와서 보관
	WireFrameMaterial = FMaterialManager::Get().FindByName(WireframeMaterialName);

	CreateGridResource(Renderer);
}

void FEditorViewportClient::CreateGridResource(FRenderer* Renderer)
{
	// 그리드 리소스 초기화
	ID3D11Device* Device = Renderer->GetDevice();
	if (Device)
	{
		// 그리드 메시 생성 (18개의 정점, SV_VertexID 호환용)
		GridMesh = std::make_unique<FMeshData>();
		GridMesh->Topology = EMeshTopology::EMT_TriangleList;
		for (int i = 0; i < 18; ++i)
		{
			FPrimitiveVertex v;
			GridMesh->Vertices.push_back(v);
			GridMesh->Indices.push_back(i);
		}
		GridMesh->CreateVertexAndIndexBuffer(Device);

		// 그리드 머티리얼 생성
		std::wstring ShaderDirW = FPaths::ShaderDir();
		std::wstring VSPath = ShaderDirW + L"AxisVertexShader.hlsl";
		std::wstring PSPath = ShaderDirW + L"AxisPixelShader.hlsl";
		auto VS = FShaderMap::Get().GetOrCreateVertexShader(Device, VSPath.c_str());
		auto PS = FShaderMap::Get().GetOrCreatePixelShader(Device, PSPath.c_str());

		GridMaterial = std::make_shared<FMaterial>();
		GridMaterial->SetOriginName("M_EditorGrid");
		GridMaterial->SetVertexShader(VS);
		GridMaterial->SetPixelShader(PS);

		FRasterizerStateOption rasterizerOption;
		rasterizerOption.FillMode = D3D11_FILL_SOLID;
		rasterizerOption.CullMode = D3D11_CULL_NONE;
		auto RS = Renderer->GetRenderStateManager()->GetOrCreateRasterizerState(rasterizerOption);
		GridMaterial->SetRasterizerOption(rasterizerOption);
		GridMaterial->SetRasterizerState(RS);

		FDepthStencilStateOption depthStencilOption;
		depthStencilOption.DepthEnable = true;
		depthStencilOption.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		auto DSS = Renderer->GetRenderStateManager()->GetOrCreateDepthStencilState(depthStencilOption);
		GridMaterial->SetDepthStencilOption(depthStencilOption);
		GridMaterial->SetDepthStencilState(DSS);

		// b2: Per-Material Constant Buffer (32 bytes)
		int32 SlotIndex = GridMaterial->CreateConstantBuffer(Device, 32);
		if (SlotIndex >= 0)
		{
			GridMaterial->RegisterParameter("GridSize", SlotIndex, 12, 4);
			GridMaterial->RegisterParameter("LineThickness", SlotIndex, 16, 4);

	
			GridMaterial->SetParameterData("GridSize", &GridSize, 4);
			GridMaterial->SetParameterData("LineThickness", &LineThickness, 4);
		}
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
	if (!Core)
	{
		return;
	}

	if (ImGui::GetCurrentContext())
	{
		const ImGuiIO& IO = ImGui::GetIO();
		if ((IO.WantCaptureKeyboard || IO.WantCaptureMouse) && !EditorUI.IsViewportInteractive())
		{
			return;
		}
	}

	if (!EditorUI.IsViewportInteractive())
	{
		return;
	}

	FViewportClient::Tick(Core, DeltaTime);
}

void FEditorViewportClient::HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (!Core || !EditorUI.IsViewportInteractive())
	{
		return;
	}

	if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse && !EditorUI.IsViewportInteractive())
	{
		return;
	}

	ULevel* Level = ResolveLevel(Core);
	AActor* SelectedActor = Core->GetSelectedActor();
	if (!Level)
	{
		return;
	}

	const bool bHasViewportMouse = EditorUI.GetViewportMousePosition(
		static_cast<int32>(static_cast<short>(LOWORD(LParam))),
		static_cast<int32>(static_cast<short>(HIWORD(LParam))),
		ScreenMouseX,
		ScreenMouseY,
		ScreenWidth,
		ScreenHeight);

	const bool bRightMouseDown = Core->GetInputManager() &&
		Core->GetInputManager()->IsMouseButtonDown(FInputManager::MOUSE_RIGHT);

	switch (Msg)
	{
	case WM_KEYDOWN:
		if (bRightMouseDown)
		{
			return;
		}

		switch (WParam)
		{
		case 'W':
			Gizmo.SetMode(EGizmoMode::Location);
			return;

		case 'E':
			Gizmo.SetMode(EGizmoMode::Rotation);
			return;

		case 'R':
			Gizmo.SetMode(EGizmoMode::Scale);
			return;

		case 'L':
			Gizmo.ToggleCoordinateSpace();
			UE_LOG("Gizmo Space: %s", Gizmo.GetCoordinateSpace() == EGizmoCoordinateSpace::Local ? "Local" : "World");
			return;

		default:
			return;
		}

	case WM_LBUTTONDOWN:
		if (!bHasViewportMouse)
		{
			return;
		}

		if (SelectedActor && Gizmo.BeginDrag(SelectedActor, Level, Picker, ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight))
		{
			return;
		}

		{
			AActor* PickedActor = Picker.PickActor(Level, ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight);
			Core->SetSelectedActor(PickedActor);
			EditorUI.SyncSelectedActorProperty();
		}
		return;

	case WM_MOUSEMOVE:
		if (!bHasViewportMouse)
		{
			Gizmo.ClearHover();
			return;
		}

		if (!Gizmo.IsDragging())
		{
			Gizmo.UpdateHover(SelectedActor, Level, Picker, ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight);
			return;
		}

		if (Gizmo.UpdateDrag(SelectedActor, Level, Picker, ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight))
		{
			EditorUI.SyncSelectedActorProperty();
		}
		return;

	case WM_LBUTTONUP:
		if (Gizmo.IsDragging())
		{
			Gizmo.EndDrag();
			if (bHasViewportMouse)
			{
				Gizmo.UpdateHover(SelectedActor, Level, Picker, ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight);
			}
			else
			{
				Gizmo.ClearHover();
			}
			EditorUI.SyncSelectedActorProperty();
		}
		return;

	default:
		return;
	}
}

void FEditorViewportClient::HandleFileDoubleClick(const FString& FilePath)
{
	FCore* Core = EditorUI.GetCore();

	if (Core)
	{
		if (FilePath.ends_with(".json"))
		{
			Core->SetSelectedActor(nullptr);
			Core->GetLevel()->ClearActors();
			bool bLoaded = FSceneSerializer::Load(Core->GetLevel(), FilePath, Core->GetRenderer()->GetDevice());

			if (bLoaded)
			{
				UE_LOG("Level loaded: %s", FilePath.c_str());
			}			
			else
			{
				MessageBoxW(
					nullptr,
					L"Level 정보가 잘못되었습니다.",
					L"Error",
					MB_OK | MB_ICONWARNING
				);
			}
		}
	}
}

void FEditorViewportClient::HandleFileDropOnViewport(const FString& FilePath)
{
	FCore* Core = EditorUI.GetCore();

	if (Core && Core->GetRenderer())
	{
		if (FilePath.ends_with(".obj"))
		{
			const FRay Ray = Picker.ScreenToRay(Core->GetLevel()->GetCamera(), ScreenMouseX, ScreenMouseY, ScreenWidth, ScreenHeight);

			AObjActor* NewActor = Core->GetLevel()->SpawnActor<AObjActor>("ObjActor");
			
			NewActor->LoadObj(Core->GetRenderer()->GetDevice(), FPaths::ToRelativePath(FilePath));
			FVector V = Ray.Origin + Ray.Direction * 5;
			NewActor->SetActorLocation(V);
		}
	}
}

void FEditorViewportClient::BuildRenderCommands(FCore* Core, ULevel* Level,
	const FFrustum& Frustum, FRenderCommandQueue& OutQueue)
{
	FViewportClient::BuildRenderCommands(Core, Level, Frustum, OutQueue);  // non-const 부모 호출

	// RenderMode 처리
	if (RenderMode == ERenderMode::Wireframe)
	{
		for (auto it = OutQueue.Commands.begin(); it != OutQueue.Commands.end(); it++)
		{
			// TODO: 아래의 if문 삭제하고 UUID 렌더러를 컴포넌트가 아닌 EditorViewportClient의 기능으로 재구현
			if(it->RenderLayer != ERenderLayer::Overlay)
				it->Material = WireFrameMaterial.get();
		}
	}

	if (!Core || !Level || !Level->GetCamera())
	{
		return;
	}

	// 그리드(Axis) 명령 삽입
	if (GridMesh && GridMaterial&&bShowGrid)
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
		Gizmo.BuildRenderCommands(GizmoTarget, Level->GetCamera(), OutQueue);
	}
}

void FEditorViewportClient::SetGridSize(float InSize)
{
	GridSize = InSize;
	if (GridMaterial)
	{
		GridMaterial->SetParameterData("GridSize",&GridSize, 4);
	}
}

void FEditorViewportClient::SetLineThickness(float InThickness)
{
	LineThickness = InThickness;
	if (GridMaterial)
	{
		GridMaterial->SetParameterData("LineThickness", &LineThickness, 4);
	}
}
