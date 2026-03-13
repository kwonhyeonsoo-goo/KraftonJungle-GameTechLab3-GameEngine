#include "Renderer.h"

CRenderer::~CRenderer()
{
    Release();
}

bool CRenderer::Initialize(HWND Hwnd, int Width, int Height)
{
    // TODO: D3D11 디바이스, 스왑체인, 렌더 타겟, 뎁스 스텐실 초기화
    return true;
}

void CRenderer::BeginFrame()
{
    // TODO: 렌더 타겟 클리어
}

void CRenderer::EndFrame()
{
    // TODO: SwapChain->Present
}

void CRenderer::Release()
{
    // TODO: COM 리소스 해제
}
