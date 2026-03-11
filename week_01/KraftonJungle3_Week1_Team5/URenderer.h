#pragma once
#include <d3d11.h>

class UShader;

class URenderer
{
public:
    explicit URenderer(HWND hWnd);
    URenderer();
    ~URenderer();

    URenderer(const URenderer&) = delete;
    URenderer& operator=(const URenderer&) = delete;
    URenderer(const URenderer&&) = delete;
    URenderer& operator=(const URenderer&&) = delete;

    bool Create(HWND hWnd);
    void Release();

    void Prepare();

    void SwapBuffer();

    void Render(ID3D11Buffer* buffer, UINT stride, UINT numVertices);

    ID3D11Device* GetDevice() const { return Device; }
    ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
    const D3D11_VIEWPORT& GetViewportInfo() const { return ViewportInfo; }

private:
    bool CreateDeviceAndSwapChain(HWND hWnd);
    void ReleaseDeviceAndSwapChain();
    bool CreateFrameBuffer();
    void ReleaseFrameBuffer();
    bool CreateRasterizerState();
    void ReleaseRasterizerState();

    void CreateShader();

private:
    // Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑 체인(Swap Chain)을 관리하기 위한 포인터들
    ID3D11Device* Device; // GPU와 통신하기 위한 Direct3D 장치
    ID3D11DeviceContext* DeviceContext; // GPU 명령 실행을 담당하는 컨텍스트
    IDXGISwapChain* SwapChain; // 프레임 버퍼를 교체하는 데 사용되는 스왑 체인

    // 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
    ID3D11Texture2D* FrameBuffer; // 화면 출력용 텍스처
    ID3D11RenderTargetView* FrameBufferRTV; // 텍스처를 렌더 타겟으로 사용하는 뷰
    ID3D11RasterizerState* RasterizerState; // 래스터라이저 상태(컬링, 채우기 모드 등 정의)

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
    D3D11_VIEWPORT ViewportInfo = {}; // 렌더링 영역을 정의하는 뷰포트 정보
};

