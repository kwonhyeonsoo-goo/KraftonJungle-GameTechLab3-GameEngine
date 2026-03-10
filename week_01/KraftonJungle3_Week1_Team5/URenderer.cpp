#include "URenderer.h"

#include "UShader.h"
#include "Utility.h"

URenderer::URenderer(HWND hWnd)
    : Device(nullptr)
    , DeviceContext(nullptr)
    , SwapChain(nullptr)
    , FrameBuffer(nullptr)
    , FrameBufferRTV(nullptr)
    , RasterizerState(nullptr)
{
    Create(hWnd);
}

URenderer::URenderer() : Device(nullptr), DeviceContext(nullptr), SwapChain(nullptr), FrameBuffer(nullptr),
                         FrameBufferRTV(nullptr),
                         RasterizerState(nullptr)
{

}

URenderer::~URenderer()
{
    Release();
}

bool URenderer::Create(HWND hWnd)
{
    // Direct3D 장치 및 스왑 체인 생성
    if (!CreateDeviceAndSwapChain(hWnd))
    {
        return false;
    }

    // 프레임 버퍼 생성
    if (!CreateFrameBuffer())
    {
        return false;
    }

    // 래스터라이저 상태 생성
	if (!CreateRasterizerState())
	{
        return false;
	}

    // 깊이 스텐실 버퍼 및 블렌드 상태는 이 코드에서는 다루지 않음

    return true;
}

void URenderer::Release()
{
    if (DeviceContext)
    {
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        DeviceContext->RSSetState(nullptr);
        DeviceContext->ClearState();
        DeviceContext->Flush();
    }

    ReleaseFrameBuffer();
    ReleaseRasterizerState();

    if (SwapChain)
    {
        SwapChain->SetFullscreenState(FALSE, nullptr);
    }

    ReleaseDeviceAndSwapChain();
}

void URenderer::Prepare()
{
    DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    DeviceContext->RSSetViewports(1, &ViewportInfo);
    DeviceContext->RSSetState(RasterizerState);

    DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
    DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void URenderer::SwapBuffer()
{
    if (SwapChain)
    {
        HRESULT hr = SwapChain->Present(1, 0);
        // 필요하면 hr 체크
    }
}

void URenderer::Render(ID3D11Buffer* buffer, UINT stride, UINT numVertices)
{
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);

    DeviceContext->Draw(numVertices, 0);
}

bool URenderer::CreateDeviceAndSwapChain(HWND hWnd)
{
    // 지원하는 Direct3D 기능 레벨을 정의
    D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // 스왑 체인 설정 구조체 초기화
    DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
    swapchaindesc.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
    swapchaindesc.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
    swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
    swapchaindesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
    swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
    swapchaindesc.BufferCount = 2; // 더블 버퍼링
    swapchaindesc.OutputWindow = hWnd; // 렌더링할 창 핸들
    swapchaindesc.Windowed = TRUE; // 창 모드
    swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

    // Direct3D 장치와 스왑 체인을 생성
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
        featurelevels,
        ARRAYSIZE(featurelevels),
        D3D11_SDK_VERSION,
        &swapchaindesc,
        &SwapChain, 
        &Device,
        nullptr, 
        &DeviceContext
    );

    if (FAILED(hr))
    {
        return false;
    }

    hr = SwapChain->GetDesc(&swapchaindesc);

    if (FAILED(hr))
    {
        return false;
    }

    ViewportInfo = {
	    .TopLeftX = 0.0f,
	    .TopLeftY = 0.0f,
	    .Width = static_cast<float>(swapchaindesc.BufferDesc.Width),
	    .Height = static_cast<float>(swapchaindesc.BufferDesc.Height),
	    .MinDepth = 0.0f,
	    .MaxDepth = 1.0f
    };

    return true;
}

void URenderer::ReleaseDeviceAndSwapChain()
{
    if (DeviceContext)
    {
        DeviceContext->Flush(); // 남아있는 GPU 명령 실행
    }

    SafeRelease(SwapChain);
    SafeRelease(DeviceContext);
    SafeRelease(Device);
}

bool URenderer::CreateFrameBuffer()
{
    ReleaseFrameBuffer();

    // 스왑 체인으로부터 백 버퍼 텍스처 가져오기
    HRESULT hr = SwapChain->GetBuffer(0, IID_PPV_ARGS(&FrameBuffer));
    if (FAILED(hr))
    {
        return false;
    }

    // 렌더 타겟 뷰 생성
    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
    framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 색상 포맷
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    hr = Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

void URenderer::ReleaseFrameBuffer()
{
    SafeRelease(FrameBufferRTV);
    SafeRelease(FrameBuffer);
}

bool URenderer::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC rasterizerdesc = {};
    rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
    rasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링

    HRESULT hr = Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

void URenderer::ReleaseRasterizerState()
{
    SafeRelease(RasterizerState);
}

void URenderer::CreateShader()
{
    D3D11_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
        D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "Color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
        D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
}