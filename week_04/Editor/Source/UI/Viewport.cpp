#include "Viewport.h"

#include "EditorViewportClient.h"
#include "Core/Core.h"
#include "Core/ViewportClient.h"
#include "Renderer/Renderer.h"
#include "Camera/ViewportInfo.h"
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
		if (!ViewportClient) return false;

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

		if (bSelected) ImGui::PopStyleColor(3);
		return bClicked;
	}
}

FViewport::~FViewport()
{
	ReleaseLevelView();
}

void FViewport::Render(FCore* Core, FRenderer* Renderer, HWND Hwnd)
{
	const bool bOpen = ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_MenuBar);

	if (!bOpen)
	{
		bHovered = false;
		bFocused = false;
		bVisible = false;
		if (Renderer) Renderer->ClearLevelRenderTarget();
		ImGui::End();
		return;
	}

	// ── 메뉴바 — 연결된 ViewportClient가 EditorViewportClient인 경우만 표시 ──
	if (ImGui::BeginMenuBar())
	{
		FEditorViewportClient* EditorVP =
			LinkedViewportClient
			? dynamic_cast<FEditorViewportClient*>(LinkedViewportClient)
			: nullptr;

		if (EditorVP)
		{
			ImGui::Separator();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y));

			if (RenderGizmoModeButton("T", EGizmoMode::Location, EditorVP))
				EditorVP->SetGizmoMode(EGizmoMode::Location);
			if (RenderGizmoModeButton("R", EGizmoMode::Rotation, EditorVP))
				EditorVP->SetGizmoMode(EGizmoMode::Rotation);
			if (RenderGizmoModeButton("S", EGizmoMode::Scale, EditorVP))
				EditorVP->SetGizmoMode(EGizmoMode::Scale);

			ImGui::PopStyleVar();

			float RenderModeComboWidth = 120.0f;
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - RenderModeComboWidth);
			ImGui::SetNextItemWidth(RenderModeComboWidth);
			{
				ERenderMode RenderMode = EditorVP->GetRenderMode();
				ImGui::Combo("", (int*)&RenderMode, "Lighting\0No Lighting\0Wireframe", 3);
				EditorVP->SetRenderMode(RenderMode);
			}
		}
		ImGui::EndMenuBar();
	}

	bFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	bHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

	// ── 크기 / 위치 계산 ──────────────────────────────────────────────────
	const ImVec2 ContentPos = ImGui::GetCursorScreenPos();
	const ImVec2 ContentSize = ImGui::GetContentRegionAvail();
	const uint32 NewWidth = ContentSize.x > 1.0f ? static_cast<uint32>(ContentSize.x) : 0;
	const uint32 NewHeight = ContentSize.y > 1.0f ? static_cast<uint32>(ContentSize.y) : 0;

	bVisible = (NewWidth > 0 && NewHeight > 0);

	if (Hwnd)
	{
		POINT ClientPoint = {
			static_cast<LONG>(ContentPos.x),
			static_cast<LONG>(ContentPos.y)
		};
		::ScreenToClient(Hwnd, &ClientPoint);
		ClientPosX = ClientPoint.x;
		ClientPosY = ClientPoint.y;
	}
	else
	{
		ClientPosX = 0;
		ClientPosY = 0;
	}

	if (!bVisible)
	{
		ReleaseLevelView();
		if (Renderer) Renderer->ClearLevelRenderTarget();
		ImGui::End();
		return;
	}

	// ── RTV/DSV 생성 ──────────────────────────────────────────────────────
	if (Renderer)
	{
		ReadyLevelView(Renderer->GetDevice(), NewWidth, NewHeight);
	}

	// ── FViewportInfo 생성 후 LinkedViewportClient에 전달 ─────────────────
	// Core::Render() 루프에서 이 정보를 읽어 D3D11_VIEWPORT 세팅 및 RTV 바인딩
	if (LinkedViewportClient && RenderTargetView && DepthStencilView)
	{
		FViewportInfo Info;
		Info.TopLeftX = 0.f;
		Info.TopLeftY = 0.f;
		Info.Width = static_cast<float>(NewWidth);
		Info.Height = static_cast<float>(NewHeight);
		Info.MinDepth = 0.f;
		Info.MaxDepth = 1.f;
		Info.ClientPosX = ClientPosX;
		Info.ClientPosY = ClientPosY;
		Info.RTV = RenderTargetView;
		Info.DSV = DepthStencilView;

		// SetViewportInfo 내부에서 OnViewportResized() 자동 호출 → AspectRatio 동기화
		LinkedViewportClient->SetViewportInfo(Info);
	}
	else if (Renderer)
	{
		Renderer->ClearLevelRenderTarget();
	}

	// ── ImGui에 결과 텍스처 표시 ──────────────────────────────────────────
	if (ShaderResourceView)
	{
		ImGui::Image(
			reinterpret_cast<ImTextureID>(ShaderResourceView),
			ImVec2(static_cast<float>(NewWidth), static_cast<float>(NewHeight)));
	}

	ImGui::End();
}

void FViewport::ReleaseLevelView()
{
	auto Release = [](auto*& Ptr)
		{
			if (Ptr) { Ptr->Release(); Ptr = nullptr; }
		};

	Release(DepthStencilView);
	Release(DepthStencilTexture);
	Release(ShaderResourceView);
	Release(RenderTargetView);
	Release(RenderTargetTexture);

	OffscreenWidth = 0;
	OffscreenHeight = 0;
}

bool FViewport::GetMousePositionInViewport(int32 WindowMouseX, int32 WindowMouseY,
	int32& OutViewportX, int32& OutViewportY,
	int32& OutWidth, int32& OutHeight) const
{
	if (!bVisible || OffscreenWidth == 0 || OffscreenHeight == 0) return false;
	if (WindowMouseX < ClientPosX || WindowMouseY < ClientPosY) return false;

	const int32 LocalX = WindowMouseX - ClientPosX;
	const int32 LocalY = WindowMouseY - ClientPosY;
	if (LocalX < 0 || LocalY < 0 ||
		LocalX >= static_cast<int32>(OffscreenWidth) ||
		LocalY >= static_cast<int32>(OffscreenHeight))
	{
		return false;
	}

	OutViewportX = LocalX;
	OutViewportY = LocalY;
	OutWidth = static_cast<int32>(OffscreenWidth);
	OutHeight = static_cast<int32>(OffscreenHeight);
	return true;
}

void FViewport::ReadyLevelView(ID3D11Device* Device, uint32 Width, uint32 Height)
{
	if (!Device) return;

	if (Width == 0 || Height == 0)
	{
		ReleaseLevelView();
		return;
	}

	if (RenderTargetView && ShaderResourceView && DepthStencilView &&
		OffscreenWidth == Width && OffscreenHeight == Height)
	{
		return; // 크기 동일 — 재생성 불필요
	}

	ReleaseLevelView();

	// Color
	D3D11_TEXTURE2D_DESC ColorDesc = {};
	ColorDesc.Width = Width;
	ColorDesc.Height = Height;
	ColorDesc.MipLevels = 1;
	ColorDesc.ArraySize = 1;
	ColorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ColorDesc.SampleDesc.Count = 1;
	ColorDesc.Usage = D3D11_USAGE_DEFAULT;
	ColorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	if (FAILED(Device->CreateTexture2D(&ColorDesc, nullptr, &RenderTargetTexture))) { ReleaseLevelView(); return; }
	if (FAILED(Device->CreateRenderTargetView(RenderTargetTexture, nullptr, &RenderTargetView))) { ReleaseLevelView(); return; }
	if (FAILED(Device->CreateShaderResourceView(RenderTargetTexture, nullptr, &ShaderResourceView))) { ReleaseLevelView(); return; }

	// Depth
	D3D11_TEXTURE2D_DESC DepthDesc = {};
	DepthDesc.Width = Width;
	DepthDesc.Height = Height;
	DepthDesc.MipLevels = 1;
	DepthDesc.ArraySize = 1;
	DepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthDesc.SampleDesc.Count = 1;
	DepthDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	if (FAILED(Device->CreateTexture2D(&DepthDesc, nullptr, &DepthStencilTexture))) { ReleaseLevelView(); return; }
	if (FAILED(Device->CreateDepthStencilView(DepthStencilTexture, nullptr, &DepthStencilView))) { ReleaseLevelView(); return; }

	OffscreenWidth = Width;
	OffscreenHeight = Height;
}