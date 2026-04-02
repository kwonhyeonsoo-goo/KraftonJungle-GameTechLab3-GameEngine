#include "EditorViewportClient.h"

#include "EditorUI.h"
#include "FEditorEngine.h"
#include "Actor/Actor.h"
#include "Actor/SkySphereActor.h"
#include "Actor/StaticMeshActor.h"
#include "Component/MeshComponent.h"
#include "Component/PrimitiveComponent.h"
#include "Component/SceneComponent.h"
#include "Core/Core.h"
#include "Core/FEngine.h"
#include "Core/Paths.h"
#include "Debug/EngineLog.h"
#include "Math/MathUtility.h"
#include "Platform/Windows/Window.h"
#include "Renderer/Material.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderStateManager.h"
#include "Renderer/ShaderMap.h"
#include "Serializer/SceneSerializer.h"
#include "ViewportWindow.h"
#include "World/Level.h"
#include "imgui.h"
#include "CameraFuction/FPersToPers.h"

#include <cmath>
#include <filesystem>

namespace
{
	constexpr float PerspectivePitchMin = -89.0f;
	constexpr float PerspectivePitchMax = 89.0f;
	constexpr float PerspectiveWheelMoveScale = 1.0f;
	constexpr float PerspectivePanMouseScale = 1.0f;
	constexpr float OrthoMinZoom = 1.0f;
	constexpr float OrthoMaxZoom = 1000.0f;
	constexpr float OrthoZoomStepScale = 1.1f;
	constexpr float ViewTransitionDuration = 0.35f;
	constexpr float NearVerticalPitchThreshold = 85.0f;
	constexpr float PerspectiveFallbackPitch = -45.0f;

	const char* GetViewportTypeLabel(EEditorViewportType ViewportType)
	{
		switch (ViewportType)
		{
		case EEditorViewportType::Top:
			return "Top";
		case EEditorViewportType::Front:
			return "Front";
		case EEditorViewportType::Right:
			return "Right";
		case EEditorViewportType::Perspective:
		default:
			return "Perspective";
		}
	}

	float LerpFloat(float A, float B, float Alpha)
	{
		return A + (B - A) * Alpha;
	}

	float ComputePerspectiveHalfWidth(float Distance, float FieldOfViewDegrees, float AspectRatio)
	{
		const float SafeDistance = FMath::Max(Distance, 0.01f);
		const float SafeAspectRatio = FMath::Max(AspectRatio, 0.01f);
		const float HalfFovRadians = FMath::DegreesToRadians(FMath::Max(FieldOfViewDegrees, 0.1f) * 0.5f);
		const float SafeTanHalfFov = FMath::Max(std::tanf(HalfFovRadians), 0.01f);
		return SafeDistance * SafeTanHalfFov * SafeAspectRatio;
	}

	float ComputeDistanceForHalfWidth(float HalfWidth, float FieldOfViewDegrees, float AspectRatio)
	{
		const float SafeHalfWidth = FMath::Max(HalfWidth, 0.01f);
		const float SafeAspectRatio = FMath::Max(AspectRatio, 0.01f);
		const float HalfFovRadians = FMath::DegreesToRadians(FMath::Max(FieldOfViewDegrees, 0.1f) * 0.5f);
		const float SafeTanHalfFov = FMath::Max(std::tanf(HalfFovRadians), 0.01f);
		return SafeHalfWidth / (SafeTanHalfFov * SafeAspectRatio);
	}

	float LerpAngleDegrees(float Start, float End, float Alpha)
	{
		float Delta = std::fmod(End - Start, 360.0f);
		if (Delta > 180.0f)
		{
			Delta -= 360.0f;
		}
		else if (Delta < -180.0f)
		{
			Delta += 360.0f;
		}

		return Start + Delta * Alpha;
	}

	const char* GetOrthoViewLabel(EEditorViewportType OrthoViewType)
	{
		switch (OrthoViewType)
		{
		case EEditorViewportType::Front:
			return "Front";
		case EEditorViewportType::Right:
			return "Right";
		case EEditorViewportType::Top:
		default:
			return "Top";
		}
	}

	FViewportContext* CreateEditorViewportContext(EEditorViewportType ViewportType)
	{
		FEditorEngine* EditorEngine = dynamic_cast<FEditorEngine*>(GEngine);
		return EditorEngine ? EditorEngine->CreateEditorViewportContext(FRect(), ViewportType) : nullptr;
	}

	SViewportWindow* CreateEditorViewportWindow(EEditorViewportType ViewportType)
	{
		FViewportContext* Context = CreateEditorViewportContext(ViewportType);
		return Context ? new SViewportWindow(FRect(), Context) : nullptr;
	}

	std::shared_ptr<FMeshData> CreateSolidWireframeMesh(ID3D11Device* Device, const FMeshData* SourceMesh)
	{
		if (!Device || !SourceMesh || SourceMesh->Topology != EMeshTopology::EMT_TriangleList)
		{
			return nullptr;
		}

		const TArray<uint32>& SourceIndices = SourceMesh->Indices;
		const TArray<FPrimitiveVertex>& SourceVertices = SourceMesh->Vertices;
		if (SourceVertices.empty() || SourceIndices.empty() || (SourceIndices.size() % 3) != 0)
		{
			return nullptr;
		}

		auto ExpandedMesh = std::make_shared<FMeshData>();
		ExpandedMesh->Topology = EMeshTopology::EMT_TriangleList;
		ExpandedMesh->Vertices.reserve(SourceIndices.size());
		ExpandedMesh->Indices.reserve(SourceIndices.size());
		ExpandedMesh->Sections = SourceMesh->Sections;

		static const FVector4 BarycentricColors[3] =
		{
			FVector4(1.0f, 0.0f, 0.0f, 1.0f),
			FVector4(0.0f, 1.0f, 0.0f, 1.0f),
			FVector4(0.0f, 0.0f, 1.0f, 1.0f)
		};

		for (size_t Index = 0; Index < SourceIndices.size(); ++Index)
		{
			const uint32 SourceVertexIndex = SourceIndices[Index];
			if (SourceVertexIndex >= SourceVertices.size())
			{
				return nullptr;
			}

			FPrimitiveVertex ExpandedVertex = SourceVertices[SourceVertexIndex];
			ExpandedVertex.Color = BarycentricColors[Index % 3];
			ExpandedMesh->Vertices.push_back(ExpandedVertex);
			ExpandedMesh->Indices.push_back(static_cast<uint32>(Index));
		}

		ExpandedMesh->UpdateLocalBound();
		if (!ExpandedMesh->CreateVertexAndIndexBuffer(Device))
		{
			return nullptr;
		}

		return ExpandedMesh;
	}
}

FEditorViewportClient::FEditorViewportClient(FEditorUI& InEditorUI, FWindow* InMainWindow, EEditorViewportType InViewportType, ELevelType InWorldType)
	: EditorUI(InEditorUI)
	, MainWindow(InMainWindow)
	, CameraViewType(InViewportType)
{
	SetWorldType(InWorldType);
	SetGridVisible(bShowGrid);

	FocusCameraFunction.SetCamera(&CameraTransform);
	ChangePersToOrthFunction.SetCamera(&CameraTransform);
	ChangeOrthoToPersFunction.SetCamera(&CameraTransform);
	ChangeOrthoToOrthoFunction.SetCamera(&CameraTransform);

	ConfigureDefaultView();
	SaveInitialCameraState();
}

void FEditorViewportClient::Attach(FCore* Core)
{
	if (!Core || !GRenderer || !MainWindow)
	{
		return;
	}

	if (!EditorUI.HasActiveViewportClient())
	{
		EditorUI.SetActiveViewportClient(this);
	}

	WireFrameMaterial = FMaterialManager::Get().FindByName(WireframeMaterialName);
	SolidWireFrameMaterial = FMaterialManager::Get().FindByName(SolidWireframeMaterialName);
	CreateGridResource(GRenderer);
#if IS_OBJ_VIEWER //뷰어에서 보여줄 normal과 uv를 준비합니다
	CreateViewerDebugMaterials(GRenderer);
#endif
}

void FEditorViewportClient::Detach()
{
	Gizmo.EndDrag();
	GridMesh.reset();
	GridMaterial.reset();
	WireFrameMaterial.reset();
	SolidWireFrameMaterial.reset();
	ViewerUVMaterial.reset();
	ViewerNormalMaterial.reset();
	SolidWireframeMeshCache.clear();

	if (EditorUI.GetActiveViewportClient() == this)
	{
		EditorUI.SetActiveViewportClient(nullptr);
	}
}

void FEditorViewportClient::CreateGridResource(FRenderer* Renderer)
{
	ID3D11Device* Device = Renderer ? Renderer->GetDevice() : nullptr;
	if (!Device)
	{
		return;
	}

	GridMesh = std::make_unique<FMeshData>();
	GridMesh->Topology = EMeshTopology::EMT_TriangleList;
	for (int i = 0; i < 18; ++i)
	{
		FPrimitiveVertex Vertex;
		GridMesh->Vertices.push_back(Vertex);
		GridMesh->Indices.push_back(i);
	}
	GridMesh->CreateVertexAndIndexBuffer(Device);

	const std::wstring ShaderDir = FPaths::ShaderDir();
	auto VertexShader = FShaderMap::Get().GetOrCreateVertexShader(Device, (ShaderDir + L"AxisVertexShader.hlsl").c_str());
	auto PixelShader = FShaderMap::Get().GetOrCreatePixelShader(Device, (ShaderDir + L"AxisPixelShader.hlsl").c_str());

	GridMaterial = std::make_shared<FMaterial>();
	GridMaterial->SetOriginName("M_EditorGrid");
	GridMaterial->SetVertexShader(VertexShader);
	GridMaterial->SetPixelShader(PixelShader);

	FRasterizerStateOption RasterizerOption;
	RasterizerOption.FillMode = D3D11_FILL_SOLID;
	RasterizerOption.CullMode = D3D11_CULL_NONE;
	GridMaterial->SetRasterizerOption(RasterizerOption);
	GridMaterial->SetRasterizerState(Renderer->GetRenderStateManager()->GetOrCreateRasterizerState(RasterizerOption));

	FDepthStencilStateOption DepthStencilOption;
	DepthStencilOption.DepthEnable = true;
	DepthStencilOption.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	GridMaterial->SetDepthStencilOption(DepthStencilOption);
	GridMaterial->SetDepthStencilState(Renderer->GetRenderStateManager()->GetOrCreateDepthStencilState(DepthStencilOption));
	FBlendStateOption BlendOption;
	BlendOption.BlendEnable = true;
	BlendOption.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendOption.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendOption.BlendOp = D3D11_BLEND_OP_ADD;
	BlendOption.SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendOption.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	BlendOption.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	GridMaterial->SetBlendState(Renderer->GetRenderStateManager()->GetOrCreateBlendState(BlendOption));
	const int32 SlotIndex = GridMaterial->CreateConstantBuffer(Device, 32);
	if (SlotIndex >= 0)
	{
		GridMaterial->RegisterParameter("GridSize", SlotIndex, 12, 4);
		GridMaterial->RegisterParameter("LineThickness", SlotIndex, 16, 4);
		GridMaterial->SetParameterData("GridSize", &GridSize, 4);
		GridMaterial->SetParameterData("LineThickness", &LineThickness, 4);
	}
}

void FEditorViewportClient::CreateViewerDebugMaterials(FRenderer* Renderer)
{
	ID3D11Device* Device = Renderer ? Renderer->GetDevice() : nullptr;
	FRenderStateManager* RenderStateManager = Renderer ? Renderer->GetRenderStateManager().get() : nullptr;
	if (!Device || !RenderStateManager)
	{
		return;
	}

	const std::wstring ShaderDir = FPaths::ShaderDir();
	auto VertexShader = FShaderMap::Get().GetOrCreateVertexShader(Device, (ShaderDir + L"StaticMeshVertexShader.hlsl").c_str());
	auto UVPixelShader = FShaderMap::Get().GetOrCreatePixelShader(Device, (ShaderDir + L"ObjViewerUVPixelShader.hlsl").c_str());
	auto NormalPixelShader = FShaderMap::Get().GetOrCreatePixelShader(Device, (ShaderDir + L"ObjViewerNormalPixelShader.hlsl").c_str());

	auto CreateDebugMaterial = [&](const char* MaterialName, const std::shared_ptr<FPixelShader>& PixelShader)
		{
			if (!VertexShader || !PixelShader)
			{
				return std::shared_ptr<FMaterial>();
			}

			auto Material = std::make_shared<FMaterial>();
			Material->SetOriginName(MaterialName);
			Material->SetVertexShader(VertexShader);
			Material->SetPixelShader(PixelShader);

			FRasterizerStateOption RasterizerOption;
			RasterizerOption.FillMode = D3D11_FILL_SOLID;
			RasterizerOption.CullMode = D3D11_CULL_NONE;
			Material->SetRasterizerOption(RasterizerOption);
			Material->SetRasterizerState(RenderStateManager->GetOrCreateRasterizerState(RasterizerOption));

			FDepthStencilStateOption DepthStencilOption;
			DepthStencilOption.DepthEnable = true;
			DepthStencilOption.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			Material->SetDepthStencilOption(DepthStencilOption);
			Material->SetDepthStencilState(RenderStateManager->GetOrCreateDepthStencilState(DepthStencilOption));

			const int32 SlotIndex = Material->CreateConstantBuffer(Device, 16);
			if (SlotIndex >= 0)
			{
				Material->RegisterParameter("ColorTint", SlotIndex, 0, 16);
				const FVector4 White(1.0f, 1.0f, 1.0f, 1.0f);
				Material->SetParameterData("ColorTint", &White, 16);
			}

			return Material;
		};

	ViewerUVMaterial = CreateDebugMaterial("M_ObjViewer_UV", UVPixelShader);
	ViewerNormalMaterial = CreateDebugMaterial("M_ObjViewer_Normal", NormalPixelShader);
}

void FEditorViewportClient::ApplyViewerNoCull(FMaterial* Material) const
{
	if (!Material || !GRenderer)
	{
		return;
	}

	const FRasterizerStateOption& CurrentOption = Material->GetRasterizerOption();
	if (CurrentOption.CullMode == D3D11_CULL_NONE)
	{
		return;
	}

	FRasterizerStateOption UpdatedOption = CurrentOption;
	UpdatedOption.CullMode = D3D11_CULL_NONE;
	Material->SetRasterizerOption(UpdatedOption);
	Material->SetRasterizerState(GRenderer->GetRenderStateManager()->GetOrCreateRasterizerState(UpdatedOption));
}

void FEditorViewportClient::ShowViewOptionPanel()
{
	if (!ImGui::CollapsingHeader("View"))
	{
		return;
	}

	FShowFlags& ShowFlags = GetShowFlags();
	auto ShowFlagCheckbox = [&ShowFlags](const char* Label, EEngineShowFlags Flag)
		{
			bool bValue = ShowFlags.HasFlag(Flag);
			if (ImGui::Checkbox(Label, &bValue))
			{
				ShowFlags.SetFlag(Flag, bValue);
			}
		};

	ImGui::SeparatorText(GetViewportLabel());

	int RenderModeIndex = static_cast<int>(GetRenderMode());
	const char* RenderModeLabels[] =
	{
		"Lighting",
		"No Lighting",
		"Wireframe",
		"Solid Wireframe",
		"UV",
		"Normals"
	};

	if (ImGui::Combo("Render Mode", &RenderModeIndex, RenderModeLabels, IM_ARRAYSIZE(RenderModeLabels)))
	{
		SetRenderMode(static_cast<ERenderMode>(RenderModeIndex));
	}

	ShowFlagCheckbox("Primitives", EEngineShowFlags::SF_Primitives);
	ShowFlagCheckbox("UUID", EEngineShowFlags::SF_UUID);
	ShowFlagCheckbox("Debug Draw", EEngineShowFlags::SF_DebugDraw);
	ShowFlagCheckbox("Grid", EEngineShowFlags::SF_Grid);
	ShowFlagCheckbox("Collision", EEngineShowFlags::SF_Collision);

	float LocalGridSize = GetGridSize();
	if (ImGui::SliderFloat("Grid Size", &LocalGridSize, 1.0f, 100.0f, "%.1f"))
	{
		SetGridSize(LocalGridSize);
	}

	float LocalThickness = GetLineThickness();
	if (ImGui::SliderFloat("Line Thickness", &LocalThickness, 0.1f, 5.0f, "%.2f"))
	{
		SetLineThickness(LocalThickness);
	}
}

UWorld* FEditorViewportClient::ResolveWorld(FCore* Core) const
{
	return FViewportClient::ResolveWorld(Core);
}

const char* FEditorViewportClient::GetViewportLabel() const
{
	return GetViewportTypeLabel(CameraViewType);
}

void FEditorViewportClient::ConfigureDefaultView()
{
	if (CameraViewType == EEditorViewportType::Perspective)
	{
		CameraTransform.SetProjectionMode(ECameraProjectionMode::Perspective);
		CameraTransform.SetPosition({ -5.0f, 0.0f, 2.0f });
		CameraTransform.SetRotation(0.0f, 0.0f);
		return;
	}

	CameraViewType = GetOrthoViewTypeFromViewportType(CameraViewType);
	CameraTransform.SetProjectionMode(ECameraProjectionMode::Orthographic);
	//CameraTransform.SetOrthoWidth(OrthoZoom);
	OrthoCenter = FVector::ZeroVector;
	OrthoViewDistance = 25.0f;

	switch (CameraViewType)
	{
	case EEditorViewportType::Top:
		CameraTransform.SetRotation(0.0f, -89.99f);
		break;
	case EEditorViewportType::Front:
		CameraTransform.SetRotation(0.0f, 0.0f);
		break;
	case EEditorViewportType::Right:
		CameraTransform.SetRotation(90.0f, 0.0f);
		break;
	}

	UpdateOrthoCameraTransform();
}

void FEditorViewportClient::DrawViewportSpecificOptions()
{
	ImGui::Spacing();
	static const char* OrthoViewLabels[] =
	{
		"Perspective",
		"Orthographic",
		"Ortho Top",
		"Ortho Front",
		"Ortho Right"
	};

	const int32 CurrentIndex =
		CameraViewType == EEditorViewportType::Perspective ? 0 :
		CameraViewType == EEditorViewportType::Orthographic ? 1 :
		CameraViewType == EEditorViewportType::Top ? 2 :
		CameraViewType == EEditorViewportType::Front ? 3 : 4;

	if (ImGui::BeginCombo("View Mode", OrthoViewLabels[CurrentIndex]))
	{
		for (int32 Index = 0; Index < IM_ARRAYSIZE(OrthoViewLabels); ++Index)
		{
			const bool bSelected = (Index == CurrentIndex);
			if (!ImGui::Selectable(OrthoViewLabels[Index], bSelected))
			{
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
				continue;
			}

			switch (Index)
			{
			case 0:
				StartPerspectiveTransition();
				break;
			case 1:
				StartOrthographicTransition();
				break;
			case 2:
				StartOrthoTransition(EEditorViewportType::Top);
				break;
			case 3:
				StartOrthoTransition(EEditorViewportType::Front);
				break;
			case 4:
				StartOrthoTransition(EEditorViewportType::Right);
				break;
			default:
				break;
			}
		}

		ImGui::EndCombo();
	}
}

void FEditorViewportClient::DrawControllerOptions()
{
	const FVector CameraPosition = CameraTransform.GetPosition();
	float Position[3] = { CameraPosition.X, CameraPosition.Y, CameraPosition.Z };
	if (ImGui::DragFloat3("Position", Position, 0.1f))
	{
		CameraTransform.SetPosition({ Position[0], Position[1], Position[2] });
	}

	float Center[3] = { OrthoCenter.X, OrthoCenter.Y, OrthoCenter.Z };
	if (ImGui::DragFloat3("Center", Center, 0.1f))
	{
		OrthoCenter = { Center[0], Center[1], Center[2] };
		UpdateOrthoCameraTransform();
	}
	if (CameraViewType == EEditorViewportType::Perspective)
	{
		float Sensitivity = CameraTransform.GetMouseSensitivity();
		if (ImGui::SliderFloat("Mouse Sensitivity", &Sensitivity, 0.01f, 1.0f))
		{
			CameraTransform.SetMouseSensitivity(Sensitivity);
		}

		float Speed = CameraTransform.GetSpeed();
		if (ImGui::SliderFloat("Move Speed", &Speed, 0.1f, 20.0f))
		{
			CameraTransform.SetSpeed(Speed);
		}


		float CameraYaw = CameraTransform.GetYaw();
		float CameraPitch = CameraTransform.GetPitch();
		bool bRotationChanged = false;
		bRotationChanged |= ImGui::DragFloat("Yaw", &CameraYaw, 0.5f);
		bRotationChanged |= ImGui::DragFloat("Pitch", &CameraPitch, 0.5f, PerspectivePitchMin, PerspectivePitchMax);
		if (bRotationChanged)
		{
			CameraTransform.SetRotation(CameraYaw, FMath::Clamp(CameraPitch, PerspectivePitchMin, PerspectivePitchMax));
		}

		float CameraFOV = CameraTransform.GetFOV();
		if (ImGui::SliderFloat("FOV", &CameraFOV, 10.0f, 120.0f))
		{
			CameraTransform.SetFOV(CameraFOV);
		}
		return;
	}

	ImGui::Text("View: %s", GetOrthoViewLabel(CameraViewType));

	float CameraYaw = CameraTransform.GetYaw();
	float CameraPitch = CameraTransform.GetPitch();
	bool bRotationChanged = false;
	bRotationChanged |= ImGui::DragFloat("Yaw", &CameraYaw, 0.5f);
	bRotationChanged |= ImGui::DragFloat("Pitch", &CameraPitch, 0.5f);
	if (bRotationChanged)
	{
		CameraTransform.SetRotation(CameraYaw, CameraPitch);
		UpdateOrthoCameraTransform();
	}

	float OrthoWidht = GetCamera()->GetOrthoWidth();
	if (ImGui::DragFloat("OrthoWidth", &OrthoWidht, 0.5f, OrthoMinZoom, OrthoMaxZoom))
	{
		UpdateOrthoCameraTransform();
	}
}

void FEditorViewportClient::ProcessCameraInput(FCore* Core, float DeltaTime)
{
	(void)Core;

	if (!InputManager)
	{
		return;
	}

	if (!ChangePersToOrthFunction.IsFinished() ||
		!ChangeOrthoToPersFunction.IsFinished() ||
		!ChangeOrthoToOrthoFunction.IsFinished())
	{
		return;
	}

	if (CameraViewType == EEditorViewportType::Perspective)
	{
		if (const float MouseWheelDelta = InputManager->GetMouseWheelDelta(); MouseWheelDelta != 0.0f)
		{
			CameraTransform.MoveForward(MouseWheelDelta * PerspectiveWheelMoveScale);
		}

		const float MouseDeltaX = InputManager->GetMouseDeltaX();
		const float MouseDeltaY = InputManager->GetMouseDeltaY();

		if (bPerspectivePanning && (MouseDeltaX != 0.0f || MouseDeltaY != 0.0f))
		{
			ApplyPerspectivePanInput(MouseDeltaX, MouseDeltaY, DeltaTime);
		}

		if (!bPerspectiveRotating)
		{
			return;
		}

		if (MouseDeltaX != 0.0f || MouseDeltaY != 0.0f)
		{
			ApplyPerspectiveLookInput(MouseDeltaX, MouseDeltaY);
		}

		float ForwardInput = 0.0f;
		float RightInput = 0.0f;
		float UpInput = 0.0f;
		if (bMoveForward) ForwardInput += 1.0f;
		if (bMoveBackward) ForwardInput -= 1.0f;
		if (bMoveRight) RightInput += 1.0f;
		if (bMoveLeft) RightInput -= 1.0f;
		if (bMoveUp) UpInput += 1.0f;
		if (bMoveDown) UpInput -= 1.0f;

		if (ForwardInput != 0.0f)
		{
			CameraTransform.MoveForward(ForwardInput * DeltaTime);
		}
		if (RightInput != 0.0f)
		{
			CameraTransform.MoveRight(RightInput * DeltaTime);
		}
		if (UpInput != 0.0f)
		{
			CameraTransform.MoveUp(UpInput * DeltaTime);
		}
		return;
	}

	if (PendingOrthoZoomStep != 0.0f)
	{
		const float ZoomScale = std::pow(OrthoZoomStepScale, PendingOrthoZoomStep);
		const float OrthoZoom = FMath::Clamp(CameraTransform.GetOrthoWidth() / ZoomScale, OrthoMinZoom, OrthoMaxZoom);
		CameraTransform.SetOrthoWidth(OrthoZoom);
		PendingOrthoZoomStep = 0.0f;
		UpdateOrthoCameraTransform();
	}

	if (!bOrthoPanning)
	{
		return;
	}

	const float MouseDeltaX = InputManager->GetMouseDeltaX();
	const float MouseDeltaY = InputManager->GetMouseDeltaY();

	const float UnitsPerPixelX = ViewportWidth > 0 ? CameraTransform.GetOrthoWidth() / static_cast<float>(ViewportWidth) : 0.0f;
	const float UnitsPerPixelY = ViewportHeight > 0 ? CameraTransform.GetOrthoHeight() / static_cast<float>(ViewportHeight) : 0.0f;
	OrthoCenter = OrthoCenter
		- GetOrthoRightVector() * (MouseDeltaX * UnitsPerPixelX)
		+ GetOrthoUpVector() * (MouseDeltaY * UnitsPerPixelY);
	UpdateOrthoCameraTransform();
}

void FEditorViewportClient::HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	(void)Hwnd;

	if (EditorUI.GetActiveViewportClient() != this)
	{
		EditorUI.SetActiveViewportClient(this);
	}

	ULevel* Level = nullptr;
	UWorld* World = nullptr;
	const bool bCanUseEditingTools = CanUseEditingTools(Core, Level, World);
	AActor* SelectedActor = bCanUseEditingTools ? GetSelectedActor() : nullptr;
	const bool bRightMouseDown = InputManager && InputManager->IsMouseButtonDown(FInputManager::MOUSE_RIGHT);

	switch (Msg)
	{
	case WM_KEYDOWN:
		HandleEditorHotkeys(WParam, bRightMouseDown);
		OnKeyDown(WParam, LParam);
		return;
	case WM_KEYUP:
		OnKeyUp(WParam, LParam);
		return;
#if IS_OBJ_VIEWER //뷰어에서만 사용하는 더블클릭입니다
	case WM_LBUTTONDBLCLK:
		ResetCameraToInitialState();
		return;
#endif
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		OnMouseButtonDown(Msg, WParam, LParam);
		if (Msg == WM_LBUTTONDOWN && bCanUseEditingTools)
		{
			HandleSelectionClick(Core, World, SelectedActor);
		}
		return;
	case WM_MOUSEMOVE:
		OnMouseMove(WParam, LParam);
		if (bCanUseEditingTools)
		{
			HandleMouseMoveForTools(SelectedActor);
		}
		return;
	case WM_MOUSEWHEEL:
		OnMouseWheel(static_cast<float>(GET_WHEEL_DELTA_WPARAM(WParam)) / static_cast<float>(WHEEL_DELTA), WParam, LParam);
		return;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		OnMouseButtonUp(Msg, WParam, LParam);
		if (Msg == WM_LBUTTONUP && bCanUseEditingTools)
		{
			HandleMouseReleaseForTools();
		}
		return;
	default:
		return;
	}
}

void FEditorViewportClient::Tick(float DeltaTime)
{
	FViewportClient::Tick(DeltaTime);
	CameraFunctionManager.Tick(DeltaTime);
#if IS_OBJ_VIEWER //더블클릭으로 카메라 위치를 초기화합니다.
	if (!bResetCameraAnimating || !bHasInitialCameraState)
	{
		return;
	}

	ResetAnimationElapsed += DeltaTime;
	const float NormalizedTime = FMath::Clamp(ResetAnimationElapsed / ResetAnimationDuration, 0.0f, 1.0f);
	const float SmoothAlpha = NormalizedTime * NormalizedTime * (3.0f - 2.0f * NormalizedTime);

	const FVector NewPosition =
		ResetAnimationStartPosition + (InitialCameraPosition - ResetAnimationStartPosition) * SmoothAlpha;
	const float NewYaw = LerpAngleDegrees(ResetAnimationStartYaw, InitialCameraYaw, SmoothAlpha);
	const float NewPitch = LerpFloat(ResetAnimationStartPitch, InitialCameraPitch, SmoothAlpha);
	const float NewFOV = LerpFloat(ResetAnimationStartFOV, InitialCameraFOV, SmoothAlpha);
	const FVector NewTarget =
		ResetAnimationStartTarget + (InitialCameraTarget - ResetAnimationStartTarget) * SmoothAlpha;

	CameraTransform.SetOrbitTarget(NewTarget);
	CameraTransform.SetPosition(NewPosition);
	CameraTransform.SetRotation(NewYaw, NewPitch);
	CameraTransform.SetFOV(NewFOV);

	if (NormalizedTime >= 1.0f)
	{
		bResetCameraAnimating = false;
		ResetAnimationElapsed = 0.0f;
	}
#endif
}

void FEditorViewportClient::SaveInitialCameraState()
{
	InitialCameraPosition = CameraTransform.GetPosition();
	InitialCameraTarget = CameraTransform.GetOrbitTarget();
	InitialCameraYaw = CameraTransform.GetYaw();
	InitialCameraPitch = CameraTransform.GetPitch();
	InitialCameraFOV = CameraTransform.GetFOV();
	bHasInitialCameraState = true;
}

void FEditorViewportClient::ResetCameraToInitialState()
{
	if (!bHasInitialCameraState)
	{
		return;
	}

	ResetAnimationStartPosition = CameraTransform.GetPosition();
	ResetAnimationStartTarget = CameraTransform.GetOrbitTarget();
	ResetAnimationStartYaw = CameraTransform.GetYaw();
	ResetAnimationStartPitch = CameraTransform.GetPitch();
	ResetAnimationStartFOV = CameraTransform.GetFOV();
	ResetAnimationElapsed = 0.0f;
	bResetCameraAnimating = true;
}

/**
 * 대상의 월드 기준 최하단 Z 값을 반환.
 *
 * \param TargetActor
 * \return
 */
float FEditorViewportClient::GetObjViewerBottomZ(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return 0.0f;
	}

	if (UPrimitiveComponent* PrimitiveComponent = TargetActor->GetComponentByClass<UPrimitiveComponent>())
	{
		const FBoxSphereBounds Bounds = PrimitiveComponent->GetWorldBounds();
		return Bounds.Center.Z - Bounds.BoxExtent.Z;
	}
	return 0.0f;
}

void FEditorViewportClient::RefreshObjViewerCameraPivot(AActor* TargetActor)
{
#if IS_OBJ_VIEWER //뷰어에서는 orbit rotate를 하므로 pivot을 대상의 중심으로 맞춰줍니다
	if (!TargetActor)
	{
		TargetActor = GetSelectedActor();
	}

	if (!TargetActor)
	{
		return;
	}

	if (UPrimitiveComponent* PrimitiveComponent = TargetActor->GetComponentByClass<UPrimitiveComponent>())
	{
		CameraTransform.SetOrbitTarget(PrimitiveComponent->GetWorldBounds().Center);
	}
#else
#endif
}

void FEditorViewportClient::FrameObjViewerCamera(AActor* TargetActor, bool bSaveInitialState)
{
	if (!TargetActor)
	{
		return;
	}

	UPrimitiveComponent* PrimitiveComponent = TargetActor->GetComponentByClass<UPrimitiveComponent>();
	if (!PrimitiveComponent)
	{
		return;
	}

	const FBoxSphereBounds Bounds = PrimitiveComponent->GetWorldBounds();
	CameraTransform.SetOrbitTarget(Bounds.Center);
	CameraTransform.SetFOV(60.0f);

	const float SafeRadius = FMath::Max(Bounds.Radius, 0.5f);
	const float HalfFovRadians = FMath::DegreesToRadians(CameraTransform.GetFOV() * 0.5f);
	const float SafeTanHalfFov = FMath::Max(std::tanf(HalfFovRadians), 0.01f);
	const float CameraDistance = FMath::Max((SafeRadius / SafeTanHalfFov) * 1.2f, SafeRadius * 2.0f);

	CameraTransform.SetPosition(Bounds.Center + FVector(-CameraDistance, 0.0f, 0.0f));
	CameraTransform.SetRotation(90.0f, 0.0f);

	if (bSaveInitialState)
	{
		SaveInitialCameraState();
	}
}

bool FEditorViewportClient::CanUseEditingTools(FCore* Core, ULevel*& OutLevel, UWorld*& OutWorld) const
{
	OutLevel = ResolveLevel(Core);
	OutWorld = ResolveWorld(Core);
	return SupportsEditingTools() && OutLevel != nullptr && OutWorld != nullptr;
}

void FEditorViewportClient::HandleEditorHotkeys(WPARAM WParam, bool bRightMouseDown)
{
	if (!ChangePersToOrthFunction.IsFinished() ||
		!ChangeOrthoToPersFunction.IsFinished() ||
		!ChangeOrthoToOrthoFunction.IsFinished())
	{
		return;
	}

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
	case 'F':
		if (!FocusCameraFunction.IsFinished())
			return;

		if (AActor* TargetActor = GetGizmoTarget())
		{
			FocusCameraFunction.FocusOnActor(TargetActor);
			CameraFunctionManager.AddFunction(&FocusCameraFunction);
		}
		return;
	default:
		return;
	}
}

void FEditorViewportClient::HandleSelectionClick(FCore* Core, UWorld* World, AActor* SelectedActor)
{
#if IS_OBJ_VIEWER //뷰어에서는 차단되는 기능
	return;
#endif

	if (SelectedActor && Gizmo.BeginDrag(SelectedActor, &CameraTransform, Picker, ViewportMouseX, ViewportMouseY, ViewportWidth, ViewportHeight))
	{
		return;
	}

	AActor* PickedActor = Picker.PickActor(World->GetAllActors(), &CameraTransform, ViewportMouseX, ViewportMouseY, ViewportWidth, ViewportHeight);
	Core->SetSelectedActor(PickedActor);
	EditorUI.SyncSelectedActorProperty();
}

void FEditorViewportClient::HandleMouseMoveForTools(AActor* SelectedActor)
{
	if (!Gizmo.IsDragging())
	{
		Gizmo.UpdateHover(SelectedActor, &CameraTransform, Picker, ViewportMouseX, ViewportMouseY, ViewportWidth, ViewportHeight);
		return;
	}

	if (Gizmo.UpdateDrag(SelectedActor, &CameraTransform, Picker, ViewportMouseX, ViewportMouseY, ViewportWidth, ViewportHeight))
	{
		EditorUI.SyncSelectedActorProperty();
	}
}

void FEditorViewportClient::HandleMouseReleaseForTools()
{
	if (!Gizmo.IsDragging())
	{
		return;
	}

	Gizmo.EndDrag();
	EditorUI.SyncSelectedActorProperty();
}

AActor* FEditorViewportClient::GetSelectedActor() const
{
	FCore* Core = EditorUI.GetCore();
	return Core ? Core->GetSelectedActor() : nullptr;
}

AActor* FEditorViewportClient::GetGizmoTarget() const
{
	AActor* SelectedActor = GetSelectedActor();
	if (!SelectedActor || SelectedActor->IsA<ASkySphereActor>())
	{
		return nullptr;
	}

	return SelectedActor;
}

void FEditorViewportClient::DrawUI()
{
	if (ViewportWindow == nullptr)
	{
		return;
	}

	FRect WindowRect = ViewportWindow->GetRect();
	ImGui::SetNextWindowPos(ImVec2(WindowRect.Position.X, WindowRect.Position.Y));

	char WindowName[128];
	sprintf_s(WindowName, "ViewportButtonFrame##%p", this);

	if (!ImGui::Begin(WindowName, nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::End();
		return;
	}
	char ItemName[128];

	sprintf_s(ItemName, "H##%p", this);
	ImGui::Text("%s Viewport", GetViewportLabel());

	const bool bHasParent = ViewportWindow->GetParent() != nullptr;
	if (!bHasParent)
	{
		sprintf_s(ItemName, "H##%p", this);
		if (ImGui::Button(ItemName))
		{
			if (SViewportWindow* NewViewportWindow = CreateEditorViewportWindow(EEditorViewportType::Perspective))
			{
				if (ViewportWindow->Split(NewViewportWindow, SplitDirection::Horizontal, SplitOption::LT))
				{
					EditorUI.SaveEditorSettings();
				}
			}
		}

		ImGui::SameLine();
		sprintf_s(ItemName, "V##%p", this);
		if (ImGui::Button(ItemName))
		{
			if (SViewportWindow* NewViewportWindow = CreateEditorViewportWindow(EEditorViewportType::Perspective))
			{
				if (ViewportWindow->Split(NewViewportWindow, SplitDirection::Vertical, SplitOption::LT))
				{
					EditorUI.SaveEditorSettings();
				}
			}
		}

		ImGui::SameLine();
		sprintf_s(ItemName, "4##%p", this);
		if (ImGui::Button(ItemName))
		{
			SViewportWindow* TopViewportWindow = CreateEditorViewportWindow(EEditorViewportType::Top);
			SViewportWindow* FrontViewportWindow = CreateEditorViewportWindow(EEditorViewportType::Front);
			SViewportWindow* RightViewportWindow = CreateEditorViewportWindow(EEditorViewportType::Right);
			if (TopViewportWindow && FrontViewportWindow && RightViewportWindow)
			{
				if (ViewportWindow->Split4(
					TopViewportWindow,
					FrontViewportWindow,
					RightViewportWindow,
					SplitOption::RB))
				{
					EditorUI.SaveEditorSettings();
				}
			}
		}
	}
	else
	{
		sprintf_s(ItemName, "-##%p", this);
		if (ImGui::Button(ItemName))
		{
			ViewportWindow->GetParent()->Merge(ViewportWindow);
			EditorUI.SaveEditorSettings();
		}
	}

	ImGui::SameLine();
	sprintf_s(ItemName, "Ortho <-> Perspective ##%p", this);
	if (ImGui::Button(ItemName))
	{
		if (CameraViewType != EEditorViewportType::Perspective)
		{
			CameraViewType = EEditorViewportType::Perspective;
			bPerspectiveRotating = false;
			bPerspectivePanning = false;
			bOrthoPanning = false;
			PendingOrthoZoomStep = 0.0f;
			ResetPerspectiveMovementState();

			ChangeOrthoToPersFunction.StartTransition(InitialCameraFOV, OrthoCenter, ViewTransitionDuration);
			CameraFunctionManager.AddFunction(&ChangeOrthoToPersFunction);
		}
		else
		{
			StartOrthographicTransition();
		}
	}
	ImGui::SameLine();
	sprintf_s(ItemName, "T##%p", this);
	if (ImGui::Button(ItemName))
	{
		StartOrthoTransition(EEditorViewportType::Top);

	}
	ImGui::SameLine();
	sprintf_s(ItemName, "F##%p", this);
	if (ImGui::Button(ItemName))
	{
		StartOrthoTransition(EEditorViewportType::Front);

	}
	ImGui::SameLine();
	sprintf_s(ItemName, "R##%p", this);
	if (ImGui::Button(ItemName))
	{
		StartOrthoTransition(EEditorViewportType::Right);
	}

	ShowViewOptionPanel();

	DrawCameraOption();
	ImGui::End();
}

void FEditorViewportClient::DrawCameraOption()
{
	ImGui::Spacing();
	if (!ImGui::CollapsingHeader("Camera"))
	{
		return;
	}

	ImGui::SeparatorText("Camera");
	DrawControllerOptions();
}

void FEditorViewportClient::StartOrthographicTransition()
{
	if (!ChangePersToOrthFunction.IsFinished() ||
		!ChangeOrthoToPersFunction.IsFinished() ||
		!ChangeOrthoToOrthoFunction.IsFinished())
	{
		return;
	}

	if (CameraTransform.GetProjectionMode() == ECameraProjectionMode::Orthographic)
	{
		CameraViewType = EEditorViewportType::Orthographic;
		return;
	}

	const FVector FocusCenter = ResolveTransitionPivot();
	const FVector CameraPosition = CameraTransform.GetPosition();
	FVector ViewDirection = CameraTransform.GetForward().GetSafeNormal();
	if (ViewDirection.IsNearlyZero())
	{
		ViewDirection = FVector::ForwardVector;
	}

	const float FocusDistance = FMath::Max(
		FVector::DotProduct(FocusCenter - CameraPosition, ViewDirection),
		0.01f);
	const float AspectRatio = FMath::Max(
		CameraTransform.GetOrthoWidth() / FMath::Max(CameraTransform.GetOrthoHeight(), 0.01f),
		0.01f);
	const float TargetHalfWidth = FMath::Max(
		ComputePerspectiveHalfWidth(FocusDistance, CameraTransform.GetFOV(), AspectRatio),
		0.25f);

	CameraViewType = EEditorViewportType::Orthographic;
	OrthoCenter = FocusCenter;

	OrthoViewDistance = ComputeDistanceForHalfWidth(TargetHalfWidth, 0.8f, AspectRatio);
	bPerspectiveRotating = false;
	bPerspectivePanning = false;
	bOrthoPanning = false;
	PendingOrthoZoomStep = 0.0f;
	ResetPerspectiveMovementState();

	ChangePersToOrthFunction.StartTransition(FocusCenter, ViewTransitionDuration);
	CameraFunctionManager.AddFunction(&ChangePersToOrthFunction);
}

void FEditorViewportClient::StartPerspectiveTransition()
{
	if (!ChangePersToOrthFunction.IsFinished() ||
		!ChangeOrthoToPersFunction.IsFinished() ||
		!ChangeOrthoToOrthoFunction.IsFinished())
	{
		return;
	}

	if (CameraViewType == EEditorViewportType::Perspective &&
		CameraTransform.GetProjectionMode() == ECameraProjectionMode::Perspective)
	{
		return;
	}


	CameraViewType = EEditorViewportType::Perspective;
	bPerspectiveRotating = false;
	bPerspectivePanning = false;
	bOrthoPanning = false;
	PendingOrthoZoomStep = 0.0f;
	ResetPerspectiveMovementState();

	ChangeOrthoToPersFunction.StartTransition(InitialCameraFOV, OrthoCenter, ViewTransitionDuration);
	CameraFunctionManager.AddFunction(&ChangeOrthoToPersFunction);
}

void FEditorViewportClient::StartOrthoTransition(EEditorViewportType InOrthoViewType)
{
	if (!ChangePersToOrthFunction.IsFinished() ||
		!ChangeOrthoToPersFunction.IsFinished() ||
		!ChangeOrthoToOrthoFunction.IsFinished())
	{
		return;
	}

	InOrthoViewType = GetOrthoViewTypeFromViewportType(InOrthoViewType);
	const bool bFromPerspective =
		CameraTransform.GetProjectionMode() == ECameraProjectionMode::Perspective ||
		CameraViewType == EEditorViewportType::Perspective;

	if (!bFromPerspective && CameraViewType == InOrthoViewType)
	{
		return;
	}

	FVector FocusCenter = CameraTransform.GetOrbitTarget();
	if (AActor* SelectedActor = GetSelectedActor())
	{
		if (UPrimitiveComponent* PrimitiveComponent = SelectedActor->GetComponentByClass<UPrimitiveComponent>())
		{
			FocusCenter = PrimitiveComponent->GetWorldBounds().Center;
		}
	}

	const FVector CameraPosition = CameraTransform.GetPosition();
	if (FVector::Dist(FocusCenter, CameraPosition) < 0.01f)
	{
		FocusCenter = CameraPosition + CameraTransform.GetForward() * OrthoViewDistance;
	}

	CameraViewType = InOrthoViewType;
	FVector TargetRotation = FVector::ZeroVector; // X=Yaw, Y=Pitch, Z=Roll

	switch (InOrthoViewType)
	{
	case EEditorViewportType::Top:
		TargetRotation = FVector(0.0f, -89.99f, 0.0f);
		break;
	case EEditorViewportType::Front:
		TargetRotation = FVector(0.0f, 0.0f, 0.0f);
		break;
	case EEditorViewportType::Right:
		TargetRotation = FVector(90.0f, 0.0f, 0.0f);
		break;
	}

	OrthoCenter = FocusCenter;
	OrthoViewDistance = FMath::Max(FVector::Dist(CameraPosition, FocusCenter), 0.01f);
	bPerspectiveRotating = false;
	bPerspectivePanning = false;
	bOrthoPanning = false;
	PendingOrthoZoomStep = 0.0f;
	ResetPerspectiveMovementState();

	//if (bFromPerspective)
	//{
	//	ChangePersToOrthFunction.StartTransition();
	//	CameraFunctionManager.AddFunction(&ChangePersToOrthFunction);
	//	return;
	//}

	ChangeOrthoToOrthoFunction.StartTransition(FocusCenter, TargetRotation, ViewTransitionDuration);
	CameraFunctionManager.AddFunction(&ChangeOrthoToOrthoFunction);
}

void FEditorViewportClient::ResetPerspectiveMovementState()
{
	bMoveForward = false;
	bMoveBackward = false;
	bMoveRight = false;
	bMoveLeft = false;
	bMoveUp = false;
	bMoveDown = false;
}

void FEditorViewportClient::ApplyPerspectiveLookInput(float MouseDeltaX, float MouseDeltaY)
{
	const float NewYaw = CameraTransform.GetYaw() + MouseDeltaX * CameraTransform.GetMouseSensitivity();
	const float NewPitch = FMath::Clamp(
		CameraTransform.GetPitch() - MouseDeltaY * CameraTransform.GetMouseSensitivity(),
		PerspectivePitchMin,
		PerspectivePitchMax);
	CameraTransform.SetRotation(NewYaw, NewPitch);
}

void FEditorViewportClient::ApplyPerspectivePanInput(float MouseDeltaX, float MouseDeltaY, float DeltaTime)
{
	const float PanRightAmount = -MouseDeltaX * DeltaTime * PerspectivePanMouseScale;
	const float PanUpAmount = MouseDeltaY * DeltaTime * PerspectivePanMouseScale;

	if (PanRightAmount != 0.0f)
	{
		CameraTransform.MoveRight(PanRightAmount);
	}

	if (PanUpAmount != 0.0f)
	{
		CameraTransform.OffsetPosition(GetViewportUpVector() * (PanUpAmount * CameraTransform.GetSpeed()));
	}
}

void FEditorViewportClient::UpdateOrthoCameraTransform()
{
	CameraTransform.SetProjectionMode(ECameraProjectionMode::Orthographic);
	CameraTransform.SetPosition(OrthoCenter - GetOrthoForwardVector() * OrthoViewDistance);
}

FVector FEditorViewportClient::GetOrthoForwardVector() const
{
	return CameraTransform.GetForward().GetSafeNormal();
}

FVector FEditorViewportClient::GetOrthoRightVector() const
{
	return CameraTransform.GetRight().GetSafeNormal();
}

FVector FEditorViewportClient::GetOrthoUpVector() const
{
	return GetViewportUpVector();
}

FVector FEditorViewportClient::ResolveTransitionPivot() const
{
	const FVector CameraPosition = CameraTransform.GetPosition();
	FVector ViewDirection = CameraTransform.GetForward().GetSafeNormal();
	if (ViewDirection.IsNearlyZero())
	{
		ViewDirection = FVector::ForwardVector;
	}

	FVector RequestedFocus = CameraTransform.GetOrbitTarget();
	if (AActor* SelectedActor = GetSelectedActor())
	{
		if (UPrimitiveComponent* PrimitiveComponent = SelectedActor->GetComponentByClass<UPrimitiveComponent>())
		{
			RequestedFocus = PrimitiveComponent->GetWorldBounds().Center;
		}
	}

	float FocusDistance = FVector::DotProduct(RequestedFocus - CameraPosition, ViewDirection);
	if (FocusDistance <= 0.01f)
	{
		const float RequestedDistance = FVector::Dist(RequestedFocus, CameraPosition);
		FocusDistance = FMath::Max(RequestedDistance, 10.0f);
	}

	return CameraPosition + ViewDirection * FocusDistance;
}

EEditorViewportType FEditorViewportClient::GetOrthoViewTypeFromViewportType(EEditorViewportType InViewportType)
{
	switch (InViewportType)
	{
	case EEditorViewportType::Front:
		return EEditorViewportType::Front;
	case EEditorViewportType::Right:
		return EEditorViewportType::Right;
	case EEditorViewportType::Top:
	default:
		return EEditorViewportType::Top;
	}
}

void FEditorViewportClient::HandleFileDoubleClick(const FString& FilePath)
{
	FCore* Core = EditorUI.GetCore();
	if (!Core || !GRenderer || !FilePath.ends_with(".json"))
	{
		return;
	}

	ULevel* Level = ResolveLevel(Core);
	if (!Level)
	{
		return;
	}

	Core->SetSelectedActor(nullptr);
	Level->ClearActors();

	if (FSceneSerializer::Load(Level, FilePath, GRenderer->GetDevice(), EditorUI.GetPerspectiveCamera()))
	{
		EditorUI.SavePerspectiveCameraInitialState();
		UE_LOG("Level loaded: %s", FilePath.c_str());
	}
	else
	{
		MessageBoxW(nullptr, L"Level information is invalid.", L"Error", MB_OK | MB_ICONWARNING);
	}
}

void FEditorViewportClient::HandleFileDropOnViewport(const FString& FilePath)
{
	FCore* Core = EditorUI.GetCore();
	if (!Core || !GRenderer || (!FilePath.ends_with(".obj") && !FilePath.ends_with(".dasset")))
	{
		return;
	}

	ULevel* Level = ResolveLevel(Core);
	if (!Level)
	{
		return;
	}

	const FRay Ray = Picker.ScreenToRay(&CameraTransform, ViewportMouseX, ViewportMouseY, ViewportWidth, ViewportHeight);
	const FVector SpawnLocation = Ray.Origin + Ray.Direction * 10.0f;

	AStaticMeshActor* MeshActor = Level->SpawnActor<AStaticMeshActor>(std::filesystem::path(FilePath).stem().string());
	if (MeshActor)
	{
		MeshActor->LoadStaticMesh(GRenderer->GetDevice(), FilePath);
		if (USceneComponent* Root = MeshActor->GetRootComponent())
		{
			FTransform Transform = Root->GetRelativeTransform();
			FVector FinalLocation = SpawnLocation;;
#if IS_OBJ_VIEWER //뷰어에서만 높이(Z) 보정값을 추가 계산
			FinalLocation.Z -= GetObjViewerBottomZ(MeshActor);
#endif
			Transform.SetLocation(FinalLocation);
			Root->SetRelativeTransform(Transform);
		}

#if IS_OBJ_VIEWER //파일에 대해 자동으로 pivot을 업데이트합니다.
		Core->SetSelectedActor(MeshActor);
		RefreshObjViewerCameraPivot(MeshActor);
		FrameObjViewerCamera(MeshActor, true);
#else
		Core->SetSelectedActor(MeshActor);
#endif
	}

	EditorUI.SyncSelectedActorProperty();
}

void FEditorViewportClient::BuildRenderCommands(TArray<AActor*>& InActors, FRenderCommandQueue& OutQueue)
{
	FViewportClient::BuildRenderCommands(InActors, OutQueue);

	if (RenderMode == ERenderMode::Wireframe)
	{
		for (FRenderCommand& Command : OutQueue.Commands)
		{
			if (Command.RenderLayer != ERenderLayer::Overlay)
			{
				Command.Material = WireFrameMaterial.get();
				Command.SortKey = FRenderCommand::MakeSortKey(Command.Material, Command.MeshData);
			}
		}
	}
	else if (RenderMode == ERenderMode::SolidWireframe)
	{
		ID3D11Device* Device = GRenderer ? GRenderer->GetDevice() : nullptr;
		for (FRenderCommand& Command : OutQueue.Commands)
		{
			if (Command.RenderLayer == ERenderLayer::Overlay || !SolidWireFrameMaterial)
			{
				continue;
			}

			Command.Material = SolidWireFrameMaterial.get();

			if (!Device || !Command.MeshData || Command.MeshData->Topology != EMeshTopology::EMT_TriangleList)
			{
				Command.SortKey = FRenderCommand::MakeSortKey(Command.Material, Command.MeshData);
				continue;
			}

			auto CachedIt = SolidWireframeMeshCache.find(Command.MeshData);
			if (CachedIt == SolidWireframeMeshCache.end())
			{
				auto ExpandedMesh = CreateSolidWireframeMesh(Device, Command.MeshData);
				CachedIt = SolidWireframeMeshCache.emplace(Command.MeshData, std::move(ExpandedMesh)).first;
			}

			if (CachedIt->second)
			{
				Command.MeshData = CachedIt->second.get();
			}

			Command.SortKey = FRenderCommand::MakeSortKey(Command.Material, Command.MeshData);
		}
	}
	else if (RenderMode == ERenderMode::UV)
	{
		for (FRenderCommand& Command : OutQueue.Commands)
		{
			if (Command.RenderLayer != ERenderLayer::Overlay && ViewerUVMaterial)
			{
				Command.Material = ViewerUVMaterial.get();
				Command.SortKey = FRenderCommand::MakeSortKey(Command.Material, Command.MeshData);
			}
		}
	}
	else if (RenderMode == ERenderMode::Normals)
	{
		for (FRenderCommand& Command : OutQueue.Commands)
		{
			if (Command.RenderLayer != ERenderLayer::Overlay && ViewerNormalMaterial)
			{
				Command.Material = ViewerNormalMaterial.get();
				Command.SortKey = FRenderCommand::MakeSortKey(Command.Material, Command.MeshData);
			}
		}
	}

#if IS_OBJ_VIEWER //뷰어에서는 강제로 Cull None을 적용합니다
	for (FRenderCommand& Command : OutQueue.Commands)
	{
		if (Command.RenderLayer != ERenderLayer::Overlay)
		{
			ApplyViewerNoCull(Command.Material);
		}
	}
#endif

	if (GridMesh && GridMaterial && IsGridVisible())
	{
		FRenderCommand GridCommand;
		GridCommand.MeshData = GridMesh.get();
		GridCommand.Material = GridMaterial.get();
		GridCommand.WorldMatrix = GetGridWorldMatrix();
		GridCommand.RenderLayer = ERenderLayer::Default;
		OutQueue.AddCommand(GridCommand);
	}

#if !IS_OBJ_VIEWER
	if (AActor* GizmoTarget = GetGizmoTarget())
	{
		Gizmo.BuildRenderCommands(GizmoTarget, &CameraTransform, OutQueue);
	}
#endif
}

void FEditorViewportClient::PostRender(FCore* Core, FRenderer* Renderer)
{
	if (!Core || !Renderer)
	{
		return;
	}

	Core->RenderStatOverlay(Renderer, GetViewportWidth(), GetViewportHeight());

#if IS_OBJ_VIEWER //뷰어에서는 하이라이트도 띄우지 않습니다.
	return;
#endif

	AActor* SelectedActor = Core->GetSelectedActor();
	if (!SelectedActor || SelectedActor->IsPendingDestroy() || !SelectedActor->IsVisible() || SelectedActor->IsA<ASkySphereActor>())
	{
		return;
	}

	if (!GetShowFlags().HasFlag(EEngineShowFlags::SF_Primitives))
	{
		return;
	}

	for (UActorComponent* Component : SelectedActor->GetComponents())
	{
		if (!Component->IsA(UPrimitiveComponent::StaticClass()))
		{
			continue;
		}

		UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
		if (PrimitiveComponent->IsA(UMeshComponent::StaticClass()))
		{
			UMeshComponent* MeshComponent = static_cast<UMeshComponent*>(PrimitiveComponent);
			if (FMeshData* MeshData = MeshComponent->GetMeshData())
			{
				Renderer->RenderOutline(MeshData, MeshComponent->GetWorldTransform());
			}
			continue;
		}

		if (PrimitiveComponent->GetPrimitive() && PrimitiveComponent->GetPrimitive()->GetMeshData())
		{
			Renderer->RenderOutline(PrimitiveComponent->GetPrimitive()->GetMeshData(), PrimitiveComponent->GetWorldTransform());
		}
	}
}

void FEditorViewportClient::SetGridSize(float InSize)
{
	GridSize = InSize;
	if (GridMaterial)
	{
		GridMaterial->SetParameterData("GridSize", &GridSize, 4);
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

bool FEditorViewportClient::IsGridVisible() const
{
	return GetShowFlags().HasFlag(EEngineShowFlags::SF_Grid);
}

void FEditorViewportClient::SetGridVisible(bool bVisible)
{
	bShowGrid = bVisible;
	GetShowFlags().SetFlag(EEngineShowFlags::SF_Grid, bVisible);
}

void FEditorViewportClient::OnMouseButtonDown(UINT Msg, WPARAM WParam, LPARAM LParam)
{
	(void)WParam;
	(void)LParam;

	if (!ChangePersToOrthFunction.IsFinished() ||
		!ChangeOrthoToPersFunction.IsFinished() ||
		!ChangeOrthoToOrthoFunction.IsFinished())
	{
		return;
	}

	if (CameraViewType == EEditorViewportType::Perspective)
	{
		if (Msg == WM_RBUTTONDOWN || Msg == WM_RBUTTONDBLCLK)
		{
			bPerspectiveRotating = true;
		}
		else if (Msg == WM_MBUTTONDOWN || Msg == WM_MBUTTONDBLCLK)
		{
			bPerspectivePanning = true;
		}
		return;
	}

	if (Msg == WM_MBUTTONDOWN || Msg == WM_MBUTTONDBLCLK || Msg == WM_RBUTTONDOWN || Msg == WM_RBUTTONDBLCLK)
	{
		bOrthoPanning = true;
	}
}

void FEditorViewportClient::OnMouseButtonUp(UINT Msg, WPARAM WParam, LPARAM LParam)
{
	(void)WParam;
	(void)LParam;

	if (!ChangePersToOrthFunction.IsFinished() ||
		!ChangeOrthoToPersFunction.IsFinished() ||
		!ChangeOrthoToOrthoFunction.IsFinished())
	{
		bPerspectiveRotating = false;
		bPerspectivePanning = false;
		bOrthoPanning = false;
		ResetPerspectiveMovementState();
		return;
	}

	if (CameraViewType == EEditorViewportType::Perspective)
	{
		if (Msg == WM_RBUTTONUP)
		{
			bPerspectiveRotating = false;
			ResetPerspectiveMovementState();
		}
		else if (Msg == WM_MBUTTONUP)
		{
			bPerspectivePanning = false;
		}
		return;
	}

	if (Msg == WM_MBUTTONUP || Msg == WM_RBUTTONUP)
	{
		bOrthoPanning = false;
	}
}

void FEditorViewportClient::OnMouseMove(WPARAM WParam, LPARAM LParam)
{
	(void)WParam;
	(void)LParam;
}

void FEditorViewportClient::OnMouseWheel(float WheelDelta, WPARAM WParam, LPARAM LParam)
{
	(void)WParam;
	(void)LParam;

	if (!ChangePersToOrthFunction.IsFinished() ||
		!ChangeOrthoToPersFunction.IsFinished() ||
		!ChangeOrthoToOrthoFunction.IsFinished())
	{
		return;
	}

	if (CameraViewType != EEditorViewportType::Perspective)
	{
		bOrthoPanning = true;
		PendingOrthoZoomStep += WheelDelta;
	}
}

void FEditorViewportClient::OnKeyDown(WPARAM WParam, LPARAM LParam)
{
	(void)LParam;

	if (!ChangePersToOrthFunction.IsFinished() ||
		!ChangeOrthoToPersFunction.IsFinished() ||
		!ChangeOrthoToOrthoFunction.IsFinished())
	{
		return;
	}

	if (CameraViewType != EEditorViewportType::Perspective)
	{
		return;
	}

	switch (WParam)
	{
	case 'W': bMoveForward = true; break;
	case 'S': bMoveBackward = true; break;
	case 'D': bMoveRight = true; break;
	case 'A': bMoveLeft = true; break;
	case 'E': bMoveUp = true; break;
	case 'Q': bMoveDown = true; break;
	default: break;
	}
}

void FEditorViewportClient::OnKeyUp(WPARAM WParam, LPARAM LParam)
{
	(void)LParam;

	if (!ChangePersToOrthFunction.IsFinished() ||
		!ChangeOrthoToPersFunction.IsFinished() ||
		!ChangeOrthoToOrthoFunction.IsFinished())
	{
		return;
	}

	if (CameraViewType != EEditorViewportType::Perspective)
	{
		return;
	}

	switch (WParam)
	{
	case 'W': bMoveForward = false; break;
	case 'S': bMoveBackward = false; break;
	case 'D': bMoveRight = false; break;
	case 'A': bMoveLeft = false; break;
	case 'E': bMoveUp = false; break;
	case 'Q': bMoveDown = false; break;
	default: break;
	}
}

FVector FEditorViewportClient::GetViewportUpVector() const
{
	const FVector Right = CameraTransform.GetRight().GetSafeNormal();
	const FVector Forward = CameraTransform.GetForward().GetSafeNormal();
	return FVector::CrossProduct(Forward, Right).GetSafeNormal();
}

FMatrix FEditorViewportClient::GetGridWorldMatrix() const
{
	AActor* Actor = GetGizmoTarget();


	FVector Translation = Actor != nullptr ? Actor->GetRootComponent() != nullptr ? Actor->GetRootComponent()->GetRelativeLocation() : FVector::ZeroVector : FVector::ZeroVector;
	FMatrix Rotation;
	FVector Scale = FVector::OneVector;
	int32 Interval = 10;

	FVector CameraLocation = CameraTransform.GetPosition();
	switch (CameraViewType)
	{
	case EEditorViewportType::Front:
		Rotation = FMatrix::MakeRotationY(FMath::DegreesToRadians(90.0f));
		Translation.Y = std::floor(CameraLocation.Y / Interval) * Interval;
		Translation.Z = std::floor(CameraLocation.Z / Interval) * Interval;
		break;

	case EEditorViewportType::Right:
		Rotation = FMatrix::MakeRotationX(FMath::DegreesToRadians(90.0f));
		Translation.X = std::floor(CameraLocation.X / Interval) * Interval;
		Translation.Z = std::floor(CameraLocation.Z / Interval) * Interval;
		break;
	case EEditorViewportType::Top:
		Translation.X = std::floor(CameraLocation.X / Interval) * Interval;
		Translation.Y = std::floor(CameraLocation.Y / Interval) * Interval;
		Rotation = FMatrix::Identity;
		break;
	case EEditorViewportType::Perspective:
		Translation.X = std::floor(CameraLocation.X / Interval) * Interval;
		Translation.Y = std::floor(CameraLocation.Y / Interval) * Interval;
		Translation.Z = 0;
		Rotation = FMatrix::Identity;
	default:
		break;
	}

	return FMatrix::MakeWorld(Translation, Rotation, Scale);
}
