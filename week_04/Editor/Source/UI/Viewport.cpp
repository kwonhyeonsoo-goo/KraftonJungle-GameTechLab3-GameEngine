#include "Viewport.h"

#include "Core/ViewportClient.h"
#include "Renderer/Renderer.h"
#include "Camera/ViewportInfo.h"

FViewport::~FViewport()
{
	ReleaseLevelView();
}

// ── PrepareAndUpdate ───────────────────────────────────────────────────────
// EditorUI가 4분할 영역 계산 후 호출
// RTV 준비 + ViewportInfo 전달만 담당 (ImGui 창 관리 없음)

void FViewport::PrepareAndUpdate(FRenderer* Renderer, HWND Hwnd,
	float RegionX, float RegionY,
	float RegionW, float RegionH)
{
	const uint32 W = RegionW > 1.f ? static_cast<uint32>(RegionW) : 0;
	const uint32 H = RegionH > 1.f ? static_cast<uint32>(RegionH) : 0;

	bVisible = (W > 0 && H > 0);

	if (!bVisible)
	{
		ReleaseLevelView();
		return;
	}

	// RTV 생성 / 갱신
	if (Renderer)
	{
		ReadyLevelView(Renderer->GetDevice(), W, H);
	}

	// 클라이언트 오프셋 — 마우스 피킹용
	if (Hwnd)
	{
		POINT Pt = { static_cast<LONG>(RegionX), static_cast<LONG>(RegionY) };
		::ScreenToClient(Hwnd, &Pt);
		ClientPosX = Pt.x;
		ClientPosY = Pt.y;
	}
	else
	{
		ClientPosX = static_cast<int32>(RegionX);
		ClientPosY = static_cast<int32>(RegionY);
	}

	// ViewportInfo 전달 — Core::Render()가 읽어서 SetLevelRenderTarget 호출
	if (LinkedViewportClient && RenderTargetView && DepthStencilView)
	{
		FViewportInfo Info;
		Info.TopLeftX = 0.f;
		Info.TopLeftY = 0.f;
		Info.Width = static_cast<float>(W);
		Info.Height = static_cast<float>(H);
		Info.MinDepth = 0.f;
		Info.MaxDepth = 1.f;
		Info.ClientPosX = ClientPosX;
		Info.ClientPosY = ClientPosY;
		Info.RTV = RenderTargetView;
		Info.DSV = DepthStencilView;

		LinkedViewportClient->SetViewportInfo(Info);
	}
}

// ── ReleaseLevelView ───────────────────────────────────────────────────────

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

// ── GetMousePositionInViewport ─────────────────────────────────────────────

bool FViewport::GetMousePositionInViewport(int32 WindowMouseX, int32 WindowMouseY,
	int32& OutViewportX, int32& OutViewportY,
	int32& OutWidth, int32& OutHeight) const
{
	if (!bVisible || OffscreenWidth == 0 || OffscreenHeight == 0) return false;
	if (WindowMouseX < ClientPosX || WindowMouseY < ClientPosY)   return false;

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

// ── ReadyLevelView ─────────────────────────────────────────────────────────

void FViewport::ReadyLevelView(ID3D11Device* Device, uint32 Width, uint32 Height)
{
	if (!Device) return;

	if (Width == 0 || Height == 0) { ReleaseLevelView(); return; }

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

	if (FAILED(Device->CreateTexture2D(&ColorDesc, nullptr, &RenderTargetTexture))) { ReleaseLevelView(); return; }
	if (FAILED(Device->CreateRenderTargetView(RenderTargetTexture, nullptr, &RenderTargetView))) { ReleaseLevelView(); return; }
	if (FAILED(Device->CreateShaderResourceView(RenderTargetTexture, nullptr, &ShaderResourceView))) { ReleaseLevelView(); return; }

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