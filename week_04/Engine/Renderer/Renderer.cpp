#include "Renderer.h"
#include "ShaderType.h"
#include "Math/MatrixUtils.h"
#include <dxgi1_3.h>
#include "Primitive/PrimitiveBase.h"
#include <cassert>
#include <algorithm>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_3.h>

struct FConstantBufferData
{
	FMatrix WVP;
	FMatrix World;
};

CRenderer::~CRenderer()
{
	Release();
}

 

void CRenderer::BeginFrame()
{
	constexpr float ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);
	DeviceContext->ClearDepthStencilView(DepthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
	DeviceContext->RSSetViewports(1, &Viewport);
	DeviceContext->RSSetState(RasterizerState);

	CommandList.clear();
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

void CRenderer::AddCommand(const FRenderCommand& Command)
{
	CommandList.push_back(Command);
}

void CRenderer::ExecuteCommands()
{
	// MeshData 포인터 기준으로 정렬 → 같은 메시끼리 묶어 State Change 최소화
	std::sort(CommandList.begin(), CommandList.end(),
		[](const FRenderCommand& A, const FRenderCommand& B)
		{
			return A.MeshData < B.MeshData;
		});

	FMeshData* CurrentMesh = nullptr;

	for (const auto& Cmd : CommandList)
	{
		if (!Cmd.MeshData)
		{
			continue;
		}

		// GPU 버퍼가 없으면 생성
		Cmd.MeshData->CreateBuffers(Device);

		// 메시가 바뀔 때만 바인딩
		if (Cmd.MeshData != CurrentMesh)
		{
			Cmd.MeshData->Bind(DeviceContext);
			CurrentMesh = Cmd.MeshData;
		}

		// 오브젝트별 상수 버퍼 업데이트
		UpdateConstantBuffer(Cmd.WorldMatrix, ViewProjectionMatrix);

		// Draw
		DeviceContext->DrawIndexed(Cmd.MeshData->IndexCount, 0, 0);
	}
}

bool CRenderer::CreateConstantBuffer()
{
	D3D11_BUFFER_DESC Desc = {};
	Desc.ByteWidth = sizeof(FConstantBufferData);
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT Hr = Device->CreateBuffer(&Desc, nullptr, &ConstantBuffer);
	if (FAILED(Hr))
	{
		MessageBox(0, L"CreateConstantBuffer Failed.", 0, 0);
		return false;
	}
	return true;
}

void CRenderer::UpdateConstantBuffer(const FMatrix& WorldMatrix, const FMatrix& ViewProj)
{
	FConstantBufferData CBData;
	CBData.World = WorldMatrix;
	CBData.WVP = WorldMatrix * ViewProj;

	D3D11_MAPPED_SUBRESOURCE Mapped;
	DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
	memcpy(Mapped.pData, &CBData, sizeof(CBData));
	DeviceContext->Unmap(ConstantBuffer, 0);

	DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
}

void CRenderer::Release()
{
	if (RasterizerState)
	{
		RasterizerState->Release();
		RasterizerState = nullptr;
	}
	if (ConstantBuffer)
	{
		ConstantBuffer->Release();
		ConstantBuffer = nullptr;
	}
	if (DepthStencilView)
	{
		DepthStencilView->Release();
		DepthStencilView = nullptr;
	}
	if (RenderTargetView)
	{
		RenderTargetView->Release();
		RenderTargetView = nullptr;
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
