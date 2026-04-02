#include "Viewport.h"

#include "FEditorEngine.h"
#include "EditorViewportClient.h"
#include "Core/Core.h"
#include "Renderer/Renderer.h"
#include "World/Level.h"
#include "Camera/Camera.h"

#include "imgui.h"

namespace
{
	void ReleaseIfValid(IUnknown*& Resource)
	{
		if (Resource)
		{
			Resource->Release();
			Resource = nullptr;
		}
	}

	bool RenderGizmoModeButton(const char* Label, EGizmoMode Mode, FEditorViewportClient* ViewportClient)
	{
		if (ViewportClient == nullptr)
		{
			return false;
		}

		const bool bSelected = (ViewportClient->GetGizmoMode() == Mode);
		const float ButtonHeight = ImGui::GetFrameHeight();
		const ImVec2 ButtonSize(ButtonHeight, ButtonHeight);
		if (bSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.45f, 0.85f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.55f, 0.95f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.40f, 0.80f, 1.0f));
		}

		const bool bClicked = ImGui::Button(Label, ButtonSize);

		if (bSelected)
		{
			ImGui::PopStyleColor(3);
		}

		return bClicked;
	}
}

FViewportLegacy::~FViewportLegacy()
{
	ReleaseLevelView();
}

void FViewportLegacy::Render(HWND Hwnd)
{
	(void)Hwnd;

	const bool bOpen = ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_MenuBar);
	if (!bOpen)
	{
		if (GRenderer)
		{
			GRenderer->ClearLevelRenderTarget();
		}
		ImGui::End();
		return;
	}
	dynamic_cast<FEditorEngine*>(GEngine)->GetWindowManager().DrawWindows();

	if (ImGui::BeginMenuBar())
	{

		/*FEditorViewportClient* EditorViewportClient = Core ? dynamic_cast<FEditorViewportClient*>(Core->GetViewportClient()) : nullptr;
		if (EditorViewportClient)
		{
			ImGui::Separator();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y));
			if (RenderGizmoModeButton("T", EGizmoMode::Location, EditorViewportClient))
			{
				EditorViewportClient->SetGizmoMode(EGizmoMode::Location);
			}
			if (RenderGizmoModeButton("R", EGizmoMode::Rotation, EditorViewportClient))
			{
				EditorViewportClient->SetGizmoMode(EGizmoMode::Rotation);
			}
			if (RenderGizmoModeButton("S", EGizmoMode::Scale, EditorViewportClient))
			{
				EditorViewportClient->SetGizmoMode(EGizmoMode::Scale);
			}
			ImGui::PopStyleVar();

			float RenderModeComboWidth = 120.0f;
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - RenderModeComboWidth);
			ImGui::SetNextItemWidth(RenderModeComboWidth);
			ERenderMode RenderMode = EditorViewportClient->GetRenderMode();
			ImGui::Combo("", reinterpret_cast<int*>(&RenderMode), "Lighting\0No Lighting\0Wireframe\0SolidWireframe", 4);
			EditorViewportClient->SetRenderMode(RenderMode);
		}*/
		ImGui::EndMenuBar();
	}

	const ImVec2 ContentSize = ImGui::GetContentRegionAvail();
	const ImVec2 ContentMin = ImGui::GetCursorScreenPos();
	const uint32 NewWidth = ContentSize.x > 1.0f ? static_cast<uint32>(ContentSize.x) : 0;
	const uint32 NewHeight = ContentSize.y > 1.0f ? static_cast<uint32>(ContentSize.y) : 0;

	//if (GRenderer)
	//{
	//	if (FEditorEngine* EditorEngine = dynamic_cast<FEditorEngine*>(GEngine))
	//	{
	//		POINT ViewportClientPoint = {
	//			static_cast<LONG>(ContentMin.x),
	//			static_cast<LONG>(ContentMin.y)
	//		};
	//		::ScreenToClient(GRenderer->GetHwnd(), &ViewportClientPoint);
	//		EditorEngine->SetViewportLayoutBounds(ViewportClientPoint.x, ViewportClientPoint.y, NewWidth, NewHeight);
	//	}
	//}

	if (NewWidth == 0 || NewHeight == 0)
	{
		ReleaseLevelView();
		if (GRenderer)
		{
			GRenderer->ClearLevelRenderTarget();
		}
		ImGui::End();
		return;
	}

	if (GRenderer)
	{
		ReadyLevelView(GRenderer->GetDevice(), NewWidth, NewHeight);

		if (RenderTargetView && DepthStencilView)
		{
			D3D11_VIEWPORT LevelViewport = {};
			LevelViewport.TopLeftX = 0.0f;
			LevelViewport.TopLeftY = 0.0f;
			LevelViewport.Width = static_cast<float>(NewWidth);
			LevelViewport.Height = static_cast<float>(NewHeight);
			LevelViewport.MinDepth = 0.0f;
			LevelViewport.MaxDepth = 1.0f;
			GRenderer->SetLevelRenderTarget(RenderTargetView, DepthStencilView, LevelViewport);
			//dynamic_cast<FEditorEngine*>(GEngine)->SetViewportLayoutBounds(0, 0, NewWidth, NewHeight);
		}
		else
		{
			GRenderer->ClearLevelRenderTarget();
		}
	}

	//if (Core && Core->GetLevel() && Core->GetLevel()->GetCamera())
	//{
	//	Core->GetLevel()->GetCamera()->SetAspectRatio(static_cast<float>(NewWidth) / static_cast<float>(NewHeight));
	//}

	if (ShaderResourceView)
	{
		ImGui::Image(reinterpret_cast<ImTextureID>(ShaderResourceView), ImVec2(static_cast<float>(NewWidth), static_cast<float>(NewHeight)));
		bImageHovered = ImGui::IsItemHovered();
	}

	ImGui::End();
}

void FViewportLegacy::ReleaseLevelView()
{
	IUnknown* Resource = reinterpret_cast<IUnknown*>(DepthStencilView);
	ReleaseIfValid(Resource);
	DepthStencilView = nullptr;

	Resource = reinterpret_cast<IUnknown*>(DepthStencilTexture);
	ReleaseIfValid(Resource);
	DepthStencilTexture = nullptr;

	Resource = reinterpret_cast<IUnknown*>(ShaderResourceView);
	ReleaseIfValid(Resource);
	ShaderResourceView = nullptr;

	Resource = reinterpret_cast<IUnknown*>(RenderTargetView);
	ReleaseIfValid(Resource);
	RenderTargetView = nullptr;

	Resource = reinterpret_cast<IUnknown*>(RenderTargetTexture);
	ReleaseIfValid(Resource);
	RenderTargetTexture = nullptr;

	OffscreenWidth = 0;
	OffscreenHeight = 0;
}

void FViewportLegacy::ReadyLevelView(ID3D11Device* Device, uint32 Width, uint32 Height)
{
	if (Device == nullptr)
	{
		return;
	}

	if (Width == 0 || Height == 0)
	{
		ReleaseLevelView();
		return;
	}

	if (RenderTargetView && ShaderResourceView && DepthStencilView &&
		OffscreenWidth == Width && OffscreenHeight == Height)
	{
		return;
	}

	ReleaseLevelView();

	D3D11_TEXTURE2D_DESC ColorDesc = {};
	ColorDesc.Width = Width;
	ColorDesc.Height = Height;
	ColorDesc.MipLevels = 1;
	ColorDesc.ArraySize = 1;
	ColorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ColorDesc.SampleDesc.Count = 1;
	ColorDesc.Usage = D3D11_USAGE_DEFAULT;
	ColorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	if (FAILED(Device->CreateTexture2D(&ColorDesc, nullptr, &RenderTargetTexture)))
	{
		ReleaseLevelView();
		return;
	}

	if (FAILED(Device->CreateRenderTargetView(RenderTargetTexture, nullptr, &RenderTargetView)))
	{
		ReleaseLevelView();
		return;
	}

	if (FAILED(Device->CreateShaderResourceView(RenderTargetTexture, nullptr, &ShaderResourceView)))
	{
		ReleaseLevelView();
		return;
	}

	D3D11_TEXTURE2D_DESC DepthDesc = {};
	DepthDesc.Width = Width;
	DepthDesc.Height = Height;
	DepthDesc.MipLevels = 1;
	DepthDesc.ArraySize = 1;
	DepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthDesc.SampleDesc.Count = 1;
	DepthDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	if (FAILED(Device->CreateTexture2D(&DepthDesc, nullptr, &DepthStencilTexture)))
	{
		ReleaseLevelView();
		return;
	}

	if (FAILED(Device->CreateDepthStencilView(DepthStencilTexture, nullptr, &DepthStencilView)))
	{
		ReleaseLevelView();
		return;
	}

	OffscreenWidth = Width;
	OffscreenHeight = Height;
}
