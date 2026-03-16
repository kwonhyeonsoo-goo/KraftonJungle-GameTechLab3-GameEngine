#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include "../Foundation/Math/FVector.h"
#include "../Foundation/Math/FMatrix.h"
#include "../Services/SerializationService.h"
#pragma comment(lib, "dxgi.lib")


struct FVertexSimple
{
    float x, y, z;    // Position
    float r, g, b, a; // Color
    float nx, ny, nz; //normal
};

struct FConstants
{
    FMatrix MVP;
    FVector Color;
    float thickness;
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
    ID3D11RasterizerState* RasterizerStateOutline = nullptr;
    ID3D11Buffer* ConstantBuffer = nullptr;
    ID3D11Buffer* OutlineConstantBuffer = nullptr;

    ID3D11Texture2D* DepthStencilBuffer = nullptr;
    ID3D11DepthStencilView* DepthStencilView = nullptr;
    ID3D11DepthStencilState* DepthStencilState = nullptr;
    ID3D11DepthStencilState* DepthStencilOutlineState = nullptr;
    ID3D11DepthStencilState* DepthStencilDisable = nullptr;

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
    D3D11_VIEWPORT ViewportInfo;

    ID3D11VertexShader* SimpleVertexShader;
    ID3D11PixelShader* SimplePixelShader;
    ID3D11InputLayout* SimpleInputLayout;
    ID3D11VertexShader* OutlineVertexShader;
    ID3D11PixelShader* OutlinePixelShader;

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
    ID3D11Buffer* vertexBufferGizmo;
    UINT numVerticesGizmo;

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
    void UpdateMVP(const FMatrix& mvp, FVector color);
    void UpdateMVP(const FMatrix& mvp, const float thickness);

#pragma endregion
public:

    void CreateShader();
    void ReleaseShader();

    void CreateConstantBuffer();
    void ReleaseConstantBuffer();

    void Release();

private:
    IDXGIOutput* DisplayInfo;

    void Prepare();

    void PrepareShader();
    void PrepareOutlineShader();

    void SwapBuffer();


    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);
    void RenderIndexedPrimitive(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, UINT indexCount);

    void CreateDeviceAndSwapChain(HWND hWindow, uint32 width, uint32 height);

    void ReleaseDeviceAndSwapChain();

    void CreateFrameBuffer();
    void ReleaseFrameBuffer();


    void CreateDepthBuffer(uint32 width, uint32 height);
    void ReleaseDepthBuffer();

    void CreatePrimitiveVertexBuffer();
    void ReleasePrimitivVertexBuffer();


    void CreateRasterizerState();

    void ReleaseRasterizerState();

    ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);
    void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer);


    ID3D11Buffer* CreateIndexBuffer(void* data, UINT size);
    void ReleaseIndexBuffer(ID3D11Buffer* vertexBuffer);

    void UpdateConstant(FConstants& param);

};

