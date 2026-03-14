#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include "../Foundation/Math/FVector.h"
#include "../Foundation/Math/FMatrix.h"
#include "../Services/SerializationService.h"


struct FVertexSimple
{
    float x, y, z;    // Position
    float r, g, b, a; // Color
};

struct FConstants
{
    //DirectX::XMMATRIX MVP;
    FMatrix MVP;
};

class RenderQueue;
class EditorSession;
struct FMatrix;
class EditorSession;

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

    ID3D11Texture2D* DepthStencilBuffer = nullptr;
    ID3D11DepthStencilView* DepthStencilView = nullptr;
    ID3D11DepthStencilState* DepthStencilState = nullptr;

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
    D3D11_VIEWPORT ViewportInfo;

    ID3D11VertexShader* SimpleVertexShader;
    ID3D11PixelShader* SimplePixelShader;
    ID3D11InputLayout* SimpleInputLayout;
    unsigned int Stride;

    ID3D11Buffer* vertexBufferSphere;
    UINT numVerticesSphere;
    ID3D11Buffer* vertexBufferCube;
    UINT numVerticesCube;
    ID3D11Buffer* vertexBufferTriangle;
    UINT numVerticesTriangle;
    ID3D11Buffer* vertexBufferRect;
    ID3D11Buffer* indexBufferRect;
    UINT numVerticesRect;UINT numIndicesRect;
    ID3D11Buffer* vertexBufferWorldAxis;
    UINT numVerticesWorldAxis;

#pragma region D3D11 Renderer 함수들
	//D3D11 Renderer 함수들
    bool Initialize(HWND hwnd, UINT width, UINT height);
    void Shutdown();

    void BeginFrame();
    void Flush(const RenderQueue& queue, const EditorSession& session);// , const EditorSession& session);
    void EndFrame();

    // WM_SIZE 이벤트 수신 시 호출
    void OnResize(UINT width, UINT height);

    // MVP 상수 버퍼 업데이트
    void UpdateMVP(const FMatrix& mvp);

#pragma endregion
public:

    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);
    void RenderIndexedPrimitive(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer);

    void CreateShader();
    void ReleaseShader();

    void CreateDeviceAndSwapChain(HWND hWindow, UINT width, UINT height);
    void ReleaseDeviceAndSwapChain();

    void CreateFrameBuffer();
    void ReleaseFrameBuffer();

    void CreateDepthBuffer(uint32 width, uint32 height);

	void ReleaseDepthBuffer();

    void CreateRasterizerState();

    void ReleaseRasterizerState();

    void Release();

    void SwapBuffer();
    void Prepare();

    void PrepareShader();

    ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);
    void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer);


    ID3D11Buffer* CreateIndexBuffer(void* data, UINT size);
    void ReleaseIndexBuffer(ID3D11Buffer* vertexBuffer);


public:



    void CreateConstantBuffer();


    void UpdateConstant(FConstants& param);

    void ReleaseConstantBuffer();
};

