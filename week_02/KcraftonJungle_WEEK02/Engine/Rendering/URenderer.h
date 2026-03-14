#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include "FVector.h"

struct FVertexSimple
{
    float x, y, z;    // Position
    float r, g, b, a; // Color
};

struct FConstants
{
    DirectX::XMMATRIX MVP;

};


class URenderer
{
public:
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;

    ID3D11Texture2D* FrameBuffer = nullptr;
    ID3D11RenderTargetView* FrameBufferRTV = nullptr;
    ID3D11RasterizerState* RasterizerState = nullptr;
    ID3D11Buffer* ConstantBuffer = nullptr;

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
    D3D11_VIEWPORT ViewportInfo;

    ID3D11VertexShader* SimpleVertexShader;
    ID3D11PixelShader* SimplePixelShader;
    ID3D11InputLayout* SimpleInputLayout;
    unsigned int Stride;

public:
    void Create(HWND hWindow);

    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);

    void CreateShader();
    void ReleaseShader();

    void CreateDeviceAndSwapChain(HWND hWindow);
    void ReleaseDeviceAndSwapChain();

    void CreateFrameBuffer();
    void ReleaseFrameBuffer();

    void CreateRasterizerState();

    void ReleaseRasterizerState();

    void Release();

    void SwapBuffer();
    void Prepare();

    void PrepareShader();

    ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);
    void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer);
public:



    void CreateConstantBuffer();


    void UpdateConstant(FConstants& param);

    void ReleaseConstantBuffer();
};

