#include "Renderer.h"
#include "ShaderType.h"
#include "Math/MatrixUtils.h"
#include <dxgi1_3.h>
#include <cassert>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
CRenderer::~CRenderer()
{
	Release();
}

bool CRenderer::Initialize(HWND Hwnd, int Width, int Height)
{

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL featureLevels = D3D_FEATURE_LEVEL_11_0;

	HRESULT hr = D3D11CreateDevice(
		0, D3D_DRIVER_TYPE_HARDWARE, 0,
		createDeviceFlags, 0, 0,
		D3D11_SDK_VERSION, &Device, &featureLevels,
		&DeviceContext
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
		return false;
	}
	if (featureLevels != D3D_FEATURE_LEVEL_11_0)
	{
		MessageBox(0, L"Direct3D Feature Level 11 unsupported", 0, 0);
		return false;
	}
	UINT m4xMsaaQuality;
	hr = Device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality);
	assert(m4xMsaaQuality > 0);

	IDXGIDevice* dxgiDevice = nullptr;
	hr = Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	if (SUCCEEDED(hr))
	{
		IDXGIAdapter* dxgiAdapter = nullptr;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		if (SUCCEEDED(hr))
		{

			IDXGIFactory2* dxgiFactory = nullptr;
			hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory);

			if (SUCCEEDED(hr))
			{

				DXGI_SWAP_CHAIN_DESC1 sd = {};
				sd.Width = Width;
				sd.Height = Height;
				sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				sd.SampleDesc.Count = 1;
				sd.SampleDesc.Quality = 0;
				sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				sd.BufferCount = 2;
				sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

				IDXGISwapChain1* swapChain1 = nullptr;
				hr = dxgiFactory->CreateSwapChainForHwnd(
					Device, Hwnd, &sd, nullptr, nullptr, &swapChain1
				);

				if (SUCCEEDED(hr))
				{

					swapChain1->QueryInterface(__uuidof(IDXGISwapChain), (void**)&SwapChain);
					swapChain1->Release();
				}

				dxgiFactory->Release();
			}
			dxgiAdapter->Release();
		}
		dxgiDevice->Release();
	}
	if (!SwapChain)
	{
		MessageBox(0, L"SwapChain creation Failed.", 0, 0);
		return false;
	}
	ID3D11Texture2D* BackBuffer = nullptr;
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	if (FAILED(hr))
	{
		MessageBox(0, L"GetBuffer Failed.", 0, 0); return false;
	}

	hr = Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTargetView);
	BackBuffer->Release();
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateRenderTargetView Failed.", 0, 0);
		return false;
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

	ID3D11Texture2D* DepthTex = nullptr;
	hr = Device->CreateTexture2D(&DepthDesc, nullptr, &DepthTex);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateTexture2D (Depth) Failed.", 0, 0);
		return false;
	}

	hr = Device->CreateDepthStencilView(DepthTex, nullptr, &DepthStencilView);
	DepthTex->Release();
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateDepthStencilView Failed.", 0, 0); return false;
	}

	Viewport.TopLeftX = 0.f;
	Viewport.TopLeftY = 0.f;
	Viewport.Width = static_cast<float>(Width);
	Viewport.Height = static_cast<float>(Height);
	Viewport.MinDepth = 0.f;
	Viewport.MaxDepth = 1.f;


	if (!ShaderManager.LoadVertexShader(Device, L"..\\Engine\\Renderer\\Shaders\\VertexShader.hlsl"))
	{
		OutputDebugStringW(L"VS Load Failed - 파일 경로 확인\n");
		return false;
	}
	if (!ShaderManager.LoadPixelShader(Device, L"..\\Engine\\Renderer\\Shaders\\PixelShader.hlsl"))
	{
		OutputDebugStringW(L"PS Load Failed - 파일 경로 확인\n");
		return false;
	}


	return true;
}

void CRenderer::BeginFrame()
{
	constexpr float ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);
	DeviceContext->ClearDepthStencilView(DepthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
	DeviceContext->RSSetViewports(1, &Viewport);

}
void CRenderer::DrawTestTriangle()
{
	static ID3D11Buffer* s_vb = nullptr;

	if (!s_vb)
	{
		struct Vertex { float x, y, z; float r, g, b, a; float nx, ny, nz; };
		Vertex verts[3] = {
			{  0.0f,  0.5f, 0.0f,  1,0,0,1,  0,0,-1 },
			{  0.5f, -0.5f, 0.0f,  0,1,0,1,  0,0,-1 },
			{ -0.5f, -0.5f, 0.0f,  0,0,1,1,  0,0,-1 },
		};
		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = sizeof(verts);
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		D3D11_SUBRESOURCE_DATA sd = { verts };
		Device->CreateBuffer(&bd, &sd, &s_vb);
	}

	// 오브젝트 Transform
	FTransform obj;
	obj.Location = { 0.0f, 0.0f, 10.0f };
	obj.Rotation = { 0.0f, 0.0f, 0.0f };
	obj.Scale = { 1.0f, 1.0f, 1.0f };

	// 카메라 — Z -5에서 원점을 바라봄
	FCamera cam;
	cam.Position = { 0.0f, 0.0f, -5.0f };
	cam.Target = { 0.0f, 0.0f,  0.0f };
	cam.Up = { 0.0f, 1.0f,  0.0f };
	cam.Fov = 45.0f;
	cam.AspectRatio = Viewport.Width / Viewport.Height;
	cam.Near = 0.1f;
	cam.Far = 1000.0f;

	FConstants cb = {};
	BuildMVP(cb, obj, cam);
	cb.HighlightColor[0] = 1.0f;
	cb.HighlightColor[3] = 1.0f;
	cb.bIsSelected = 0;

	UINT stride = sizeof(float) * 10, offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &s_vb, &stride, &offset);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ShaderManager.Bind(DeviceContext);
	ShaderManager.UpdateConstants(DeviceContext, cb);

	DeviceContext->Draw(3, 0);
}
void CRenderer::EndFrame()
{
	SwapChain->Present(1, 0);

}

void CRenderer::Release()
{
	if (DepthStencilView)
	{
		DepthStencilView->Release();
		DepthStencilView = nullptr;
	}
	if (RenderTargetView)
	{
		RenderTargetView->Release(); RenderTargetView = nullptr;
	}
	if (SwapChain)
	{
		SwapChain->Release();
		SwapChain = nullptr;
	}
	if (DeviceContext)
	{
		DeviceContext->Release();
		DeviceContext = nullptr;
	}
	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}
	ShaderManager.Release();
}

bool CRenderer::IsOccluded()
{
	if (bSwapChainOccluded &&
		SwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		return true;

	bSwapChainOccluded = false;
	return false;
}

void CRenderer::OnResize(int NewWidth, int NewHeight)
{
	if (NewWidth == 0 || NewHeight == 0) return;


	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	RenderTargetView->Release(); RenderTargetView = nullptr;
	DepthStencilView->Release(); DepthStencilView = nullptr;

	SwapChain->ResizeBuffers(0, NewWidth, NewHeight, DXGI_FORMAT_UNKNOWN, 0);


	ID3D11Texture2D* BackBuffer = nullptr;
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTargetView);
	BackBuffer->Release();

	D3D11_TEXTURE2D_DESC DepthDesc = {};
	DepthDesc.Width = NewWidth;
	DepthDesc.Height = NewHeight;
	DepthDesc.MipLevels = 1;
	DepthDesc.ArraySize = 1;
	DepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthDesc.SampleDesc.Count = 1;
	DepthDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	ID3D11Texture2D* DepthTex = nullptr;
	Device->CreateTexture2D(&DepthDesc, nullptr, &DepthTex);
	Device->CreateDepthStencilView(DepthTex, nullptr, &DepthStencilView);
	DepthTex->Release();

	// Viewport 갱신
	Viewport.Width = static_cast<float>(NewWidth);
	Viewport.Height = static_cast<float>(NewHeight);
}



