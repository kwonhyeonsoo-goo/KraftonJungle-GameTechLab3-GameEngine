#include "URenderer.h"
#include "RenderQueue.h"
#include "../Foundation/Math/FMatrix.h"
#include <DirectXMath.h>
#include "../Mesh/Sphere.h"
#include "../Mesh/Cube.h"
#include "../Mesh/Triangle.h"
#include "../Mesh/Rect.h"
#include "../Mesh/Line.h"
#include "../Mesh/Gizmo.h"
#include "../Mesh/UVRect.h"
#include "../Editor/EditorSession.h"
#include "../Mesh/Torus.h"

void URenderer::RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)

{
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);

    DeviceContext->Draw(numVertices, 0);
}

void URenderer::RenderIndexedPrimitive(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, UINT indexCount)
{
    if (!vertexBuffer || !indexBuffer)
    {
        return;
    }

	UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &Stride, &offset);

    DeviceContext->IASetIndexBuffer(indexBuffer,DXGI_FORMAT_R32_UINT, offset);

	DeviceContext->DrawIndexed(indexCount, offset, 0);

}

void URenderer::RenderIndexedPrimitive(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, UINT indexCount, UINT stride)
{
    if (!vertexBuffer || !indexBuffer)
    {
        return;
    }

    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

    DeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, offset);

    DeviceContext->DrawIndexed(indexCount, offset, 0);
}

void URenderer::CreateShader()

{
    ID3DBlob* vertexshaderCSO = nullptr;
    ID3DBlob* pixelshaderCSO = nullptr;

    D3DCompileFromFile(L"Shaders/ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexshaderCSO, nullptr);

    Device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

    D3DCompileFromFile(L"Shaders/ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelshaderCSO, nullptr);

    Device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        // POSITION: x, y, z (3 * 4 = 12 bytes) -> 시작점: 0
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        // COLOR: r, g, b, a (4 * 4 = 16 bytes) -> 시작점: 12
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        // NORMAL: nx, ny, nz (3 * 4 = 12 bytes) -> 시작점: 12 + 16 = 28
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &SimpleInputLayout);

    Stride = sizeof(FVertexSimple);

    vertexshaderCSO->Release();
    pixelshaderCSO->Release();

    ID3DBlob* vertexShaderOutlineCS0 = nullptr;
    ID3DBlob* pixelShaderOutlineCS0 = nullptr;

    D3DCompileFromFile(L"Shaders/ShaderOutline.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexShaderOutlineCS0, nullptr);

    Device->CreateVertexShader(vertexShaderOutlineCS0->GetBufferPointer(), vertexShaderOutlineCS0->GetBufferSize(), nullptr, &OutlineVertexShader);

    D3DCompileFromFile(L"Shaders/ShaderOutline.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelShaderOutlineCS0, nullptr);

    Device->CreatePixelShader(pixelShaderOutlineCS0->GetBufferPointer(), pixelShaderOutlineCS0->GetBufferSize(), nullptr, &OutlinePixelShader);

    Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderOutlineCS0->GetBufferPointer(), vertexShaderOutlineCS0->GetBufferSize(), &SimpleInputLayout);

    vertexShaderOutlineCS0->Release();
    pixelShaderOutlineCS0->Release();

    ID3DBlob* vertexshaderCS2 = nullptr;
    ID3DBlob* pixelshaderCS2= nullptr;

    StrideUV = sizeof(FVertexUV);

    D3DCompileFromFile(L"Shaders/ShaderWGrid.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexshaderCS2, nullptr);

    Device->CreateVertexShader(vertexshaderCS2->GetBufferPointer(), vertexshaderCS2->GetBufferSize(), nullptr, &GridVertexShader);

    D3DCompileFromFile(L"Shaders/ShaderWGrid.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelshaderCS2, nullptr);

    Device->CreatePixelShader(pixelshaderCS2->GetBufferPointer(), pixelshaderCS2->GetBufferSize(), nullptr, &GridPixelShader);

    D3D11_INPUT_ELEMENT_DESC layout2[] =
    {
        // POSITION: x, y, z (3 * 4 = 12 bytes) -> 시작점: 0
        { "POSITION", 0,    DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        // COLOR: r, g, b, a (2 * 4 = 8 bytes) -> 시작점: 12
        { "TEXCOORD", 0,   DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        // NORMAL: nx, ny, nz (3 * 4 = 12 bytes) -> 시작점: 12 + 8 = 20
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    Device->CreateInputLayout(layout2, ARRAYSIZE(layout2), vertexshaderCS2->GetBufferPointer(), vertexshaderCS2->GetBufferSize(), &GridInputLayout);

    StrideUV = sizeof(FVertexUV);



}

void URenderer::ReleaseShader()

{
    if (SimpleInputLayout)
    {
        SimpleInputLayout->Release();
        SimpleInputLayout = nullptr;
    }

    if (SimplePixelShader)
    {
        SimplePixelShader->Release();
        SimplePixelShader = nullptr;
    }

    if (SimpleVertexShader)
    {
        SimpleVertexShader->Release();
        SimpleVertexShader = nullptr;
    }
}

void URenderer::CreateDeviceAndSwapChain(HWND hWindow, UINT width, UINT height)
{
    D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

    DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
    swapchaindesc.BufferDesc.Width = width;
    swapchaindesc.BufferDesc.Height = height;
    swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapchaindesc.SampleDesc.Count = 1;
    swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchaindesc.BufferCount = 2;
    swapchaindesc.OutputWindow = hWindow;
    swapchaindesc.Windowed = TRUE;
    swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
        featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
        &swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

    SwapChain->GetDesc(&swapchaindesc);

    ViewportInfo = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
    }

void URenderer::ReleaseDeviceAndSwapChain()
{
    if (DeviceContext)
    {
        DeviceContext->Flush();
    }

    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }

    if (DeviceContext)
    {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }
}

void URenderer::CreateFrameBuffer()
{
    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
    framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);

    D3D11_TEXTURE2D_DESC desc;
    FrameBuffer->GetDesc(&desc);

    // 이제 사이즈를 사용할 수 있습니다.
    UINT width = desc.Width;   // 에러 로그에 찍혔던 1008
    UINT height = desc.Height; // 에러 로그에 찍혔던 985

    CreateDepthBuffer(desc.Width, desc.Height);
}

void URenderer::ReleaseFrameBuffer()
{
    if (FrameBuffer)
    {
        FrameBuffer->Release();
        FrameBuffer = nullptr;
    }

    if (FrameBufferRTV)
    {
        FrameBufferRTV->Release();
        FrameBufferRTV = nullptr;
    }
}

void URenderer::CreateDepthBuffer(uint32 width, uint32 height)

{
    D3D11_TEXTURE2D_DESC descDepth={};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    Device->CreateTexture2D(&descDepth, NULL, &DepthStencilBuffer);

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};

    // Depth test parameters
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    // Stencil test parameters
    dsDesc.StencilEnable = true;
    dsDesc.StencilReadMask = 0xFF;
    dsDesc.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing
    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing
    dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Create depth stencil state
    Device->CreateDepthStencilState(&dsDesc, &DepthStencilState);

    D3D11_DEPTH_STENCIL_DESC dsDesc1 = {};

    // Depth test parameters
    dsDesc1.DepthEnable = true;
    dsDesc1.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc1.DepthFunc = D3D11_COMPARISON_LESS;
    // Stencil test parameters
    dsDesc1.StencilEnable = true;
    dsDesc1.StencilReadMask = 0xFF;
    dsDesc1.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    // ✅ 핵심: LESS 대신 LESS_EQUAL을 사용합니다.
    dsDesc1.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    dsDesc1.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing
    dsDesc1.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc1.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    dsDesc1.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc1.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing
    dsDesc1.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc1.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    dsDesc1.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc1.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Create depth stencil state
    Device->CreateDepthStencilState(&dsDesc1, &DepthStencilOutlineState);
    

    D3D11_DEPTH_STENCIL_DESC dsDescOff = {};
    dsDescOff.DepthEnable = FALSE; // ← 비활성화
    dsDescOff.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDescOff.DepthFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc.StencilEnable = true;
    Device->CreateDepthStencilState(&dsDescOff, &DepthStencilDisable);


    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;

    // Create the depth stencil view
    Device->CreateDepthStencilView(DepthStencilBuffer, // Depth stencil texture
        &descDSV, // Depth stencil desc
        &DepthStencilView);  // [out] Depth stencil view

}

void URenderer::ReleaseDepthBuffer()
{
    if (DepthStencilBuffer)
    {
        DepthStencilBuffer->Release();
        DepthStencilBuffer = nullptr;
    };
    if (DepthStencilView) {
		DepthStencilView->Release();
		DepthStencilView = nullptr;
    }
    if (DepthStencilState) {
        DepthStencilState->Release();
        DepthStencilState = nullptr;
    }

    if (DepthStencilOutlineState) {
        DepthStencilOutlineState->Release();
        DepthStencilOutlineState = nullptr;
    }
}

void URenderer::CreatePrimitiveVertexBuffer()
{
    for (int i = 0; i < 2400; i ++ ) {
        sphere_vertices[i].nx = sphere_vertices[i].x;
        sphere_vertices[i].ny = sphere_vertices[i].y;
        sphere_vertices[i].nz = sphere_vertices[i].z;
    }

    vertexBufferSphere = CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));
    numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);

    vertexBufferCube = CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));
    numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexSimple);

    vertexBufferTriangle = CreateVertexBuffer(triangle_vertices, sizeof(triangle_vertices));
    numVerticesTriangle = sizeof(triangle_vertices) / sizeof(FVertexSimple);

    vertexBufferRect = CreateVertexBuffer(rect_vertices, sizeof(rect_vertices));
    indexBufferRect = CreateIndexBuffer(rect_indices, sizeof(rect_indices));

    vertexBufferWorldAxis = CreateVertexBuffer(line_vertices, sizeof(line_vertices));
    numVerticesWorldAxis = sizeof(line_vertices) / sizeof(FVertexSimple);

    vertexBufferGizmo = CreateVertexBuffer(gizmoVertices, sizeof(gizmoVertices));
    numVerticesGizmo = sizeof(gizmoVertices) / sizeof(FVertexSimple);
    FVertexSimple Torus[TORUS_VERTEX_COUNT];
    GetTorusVertices(Torus);
    vertexBufferTorus = CreateVertexBuffer(Torus, sizeof(Torus));
    numVerticesTorus = sizeof(Torus) / sizeof(FVertexSimple);

    vertexBufferGrid = CreateVertexBuffer(UVrect_vertices, sizeof(UVrect_vertices));
    indexBufferGrid = CreateIndexBuffer(UVrect_indices, sizeof(UVrect_indices));

}

void URenderer::CreateRasterizerState()
{

    D3D11_RASTERIZER_DESC rasterizerdesc = {};
    rasterizerdesc.FillMode = D3D11_FILL_SOLID;
    rasterizerdesc.CullMode = D3D11_CULL_BACK;


    rasterizerdesc.FrontCounterClockwise = false;
    Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);

    D3D11_RASTERIZER_DESC rasterozeroutlinedesc = {};
    rasterozeroutlinedesc.FillMode = D3D11_FILL_SOLID;
    rasterozeroutlinedesc.CullMode = D3D11_CULL_FRONT;

    Device->CreateRasterizerState(&rasterozeroutlinedesc, &RasterizerStateOutline);

}

void URenderer::ReleaseRasterizerState()
{
    if (RasterizerState)
    {
        RasterizerState->Release();
        RasterizerState = nullptr;
    }

    if (RasterizerStateOutline) {
        RasterizerStateOutline->Release();
        RasterizerStateOutline = nullptr;
    }
}

void URenderer::Release()
{
    RasterizerState->Release();

    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ReleaseFrameBuffer();
    ReleaseDepthBuffer();
    ReleaseDeviceAndSwapChain();
}

void URenderer::OnResize(UINT width, UINT height)
{
    DeviceContext->OMSetRenderTargets(0, NULL, NULL);

    Engine::SafeRelease(FrameBuffer);
    Engine::SafeRelease(FrameBufferRTV);
    Engine::SafeRelease(DepthStencilBuffer);
    Engine::SafeRelease(DepthStencilView);

    HRESULT hr = SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    if (SUCCEEDED(hr)) {

        // 4. 스왑 체인으로부터 '새로운' 백 버퍼(Texture2D)를 받아와야 합니다. (매우 중요!)
        hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

        D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
        framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;


        if (SUCCEEDED(hr)) {
            // 5. 받아온 텍스처를 바탕으로 RTV 생성
            // DESC를 NULL로 주면 텍스처의 설정을 그대로 따라갑니다. (특별한 포맷 변환이 없다면 권장)
            Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
        }

        CreateDepthBuffer(width, height);
        DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, DepthStencilView);

        // 6. 뷰포트도 갱신해주어야 화면이 안 찌그러집니다.
        ViewportInfo.Width = (float)width;
        ViewportInfo.Height = (float)height;
        ViewportInfo.TopLeftX = 0;
        ViewportInfo.TopLeftY = 0;
        ViewportInfo.MinDepth = 0.0f;
        ViewportInfo.MaxDepth = 1.0f;
        DeviceContext->RSSetViewports(1, &ViewportInfo);
    }
}


void URenderer::SwapBuffer()
{
    SwapChain->Present(1, 0);
}

void URenderer::Prepare()
{
    // Bind depth stencil state
    DeviceContext->OMSetDepthStencilState(DepthStencilState, 1);


    DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, DepthStencilView);

    DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);


    DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);
    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    DeviceContext->RSSetViewports(1, &ViewportInfo);
    DeviceContext->RSSetState(RasterizerState);
}

void URenderer::PrepareShader()
{
    DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
    DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
    DeviceContext->IASetInputLayout(SimpleInputLayout);

    if (ConstantBuffer)
    {
        DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
        DeviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer);
    }

}

void URenderer::PrepareOutlineShader()
{
    DeviceContext->VSSetShader(OutlineVertexShader, nullptr, 0);
    DeviceContext->PSSetShader(OutlinePixelShader, nullptr, 0);
    DeviceContext->IASetInputLayout(SimpleInputLayout);

    if (OutlineConstantBuffer)
    {
        DeviceContext->VSSetConstantBuffers(0, 1, &OutlineConstantBuffer);
        DeviceContext->PSSetConstantBuffers(0, 1, &OutlineConstantBuffer);
    }

}
void URenderer::PrepareShader(ID3D11VertexShader* vertextShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* layout, ID3D11Buffer* contantBuffer)
{
    DeviceContext->VSSetShader(vertextShader, nullptr, 0);
    DeviceContext->PSSetShader(pixelShader, nullptr, 0);
    DeviceContext->IASetInputLayout(layout);

    if (contantBuffer)
    {
        DeviceContext->VSSetConstantBuffers(0, 1, &contantBuffer);
        DeviceContext->PSSetConstantBuffers(0, 1, &contantBuffer);
    }
}

ID3D11Buffer* URenderer::CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
{
    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = byteWidth;
    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

    ID3D11Buffer* vertexBuffer;

    Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);

    return vertexBuffer;
}

ID3D11Buffer* URenderer::CreateVertexBuffer(FVertexUV* vertices, UINT byteWidth)
{
    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = byteWidth;
    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

    ID3D11Buffer* vertexBuffer;

    Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);

    return vertexBuffer;
}



void URenderer::ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
{

    if(vertexBuffer)
        vertexBuffer->Release();
}

ID3D11Buffer* URenderer::CreateIndexBuffer(void* data, UINT size)
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = size;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = data;

    ID3D11Buffer* buffer = nullptr;
    Device->CreateBuffer(&desc, &init, &buffer);

    return buffer;
}
void URenderer::ReleaseIndexBuffer(ID3D11Buffer* vertexBuffer)
{
    vertexBuffer->Release();
}

void URenderer::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0;
    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);

    D3D11_BUFFER_DESC constantbufferdesc1 = {};
    constantbufferdesc1.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0;
    constantbufferdesc1.Usage = D3D11_USAGE_DYNAMIC;
    constantbufferdesc1.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc1.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    Device->CreateBuffer(&constantbufferdesc1, nullptr, &OutlineConstantBuffer);

    D3D11_BUFFER_DESC constantbufferdesc2 = {};
    constantbufferdesc2.ByteWidth = sizeof(FcontantsMV) + 0xf & 0xfffffff0;
    constantbufferdesc2.Usage = D3D11_USAGE_DYNAMIC;
    constantbufferdesc2.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc2.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    Device->CreateBuffer(&constantbufferdesc2, nullptr, &GridConstantBuffer);

}

void URenderer::UpdateConstant(FConstants& param)
{

    if (ConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

        DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        FConstants* constants = (FConstants*)constantbufferMSR.pData;
        {
            constants->MVP = param.MVP;
        }
        DeviceContext->Unmap(ConstantBuffer, 0);
    }
}

void URenderer::ReleaseConstantBuffer()
{
    if (ConstantBuffer)
    {
        ConstantBuffer->Release();
        ConstantBuffer = nullptr;
    }

    if (OutlineConstantBuffer) {
        OutlineConstantBuffer -> Release();
        OutlineConstantBuffer = nullptr;
    }
}


#pragma region D3D11 Renderer 함수들

bool URenderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    CreateDeviceAndSwapChain(hwnd, width, height);

    CreateFrameBuffer();

    CreateDepthBuffer(width, height);


    CreateRasterizerState();

    CreatePrimitiveVertexBuffer();

    FVertexSimple Torus[TORUS_VERTEX_COUNT];
    GetTorusVertices(Torus);
    vertexBufferTorus = CreateVertexBuffer(Torus, sizeof(Torus));
    numVerticesTorus = sizeof(Torus) / sizeof(FVertexSimple);

    CreateShader();
    CreateConstantBuffer();

    return true;
}

void URenderer::Shutdown()
{
    ReleaseConstantBuffer();
    ReleaseShader();

    if (vertexBufferSphere) { vertexBufferSphere->Release();    vertexBufferSphere = nullptr; }
    if (vertexBufferCube) { vertexBufferCube->Release();      vertexBufferCube = nullptr; }
    if (vertexBufferTriangle) { vertexBufferTriangle->Release();  vertexBufferTriangle = nullptr; }
    if (vertexBufferRect) { vertexBufferRect->Release();      vertexBufferRect = nullptr; }
    if (indexBufferRect) { indexBufferRect->Release();       indexBufferRect = nullptr; }
    if (vertexBufferWorldAxis) { vertexBufferWorldAxis->Release(); vertexBufferWorldAxis = nullptr; }
    if (vertexBufferGizmo) { vertexBufferGizmo->Release(); vertexBufferGizmo = nullptr; }
    ReleaseRasterizerState();
    ReleaseDepthBuffer();
    ReleaseFrameBuffer();
    ReleaseDeviceAndSwapChain();
}

void URenderer::BeginFrame()
{
    //렌더타겟 설정
    //화면 클리어 
    //기본 렌더 상태 설정

    Prepare();
    PrepareShader();
}

enum ETypePrimitive
{
    EPT_Triangle,
    EPT_Cube,
    EPT_Sphere,
    EPT_Rect,
    EPT_Max,
};
enum class EPrimitiveShape
{
    Cube,
    Sphere,
    Plane,
    Triangle
};

void URenderer::Flush(const RenderQueue& queue, const EditorSession& session)//, const EditorSession& session)
{

    if (queue.IsEmpty()) return;

    const FMatrix MViewPersp = session.GetViewProjMatrix();
    const FMatrix MViewOrtho = session.GetViewOrthoMatrix();

    for (int i = 0; i < queue.GetCommands().size(); ++i)
    {
        const RenderCommand& cmd = queue.GetCommands()[i];

        const FMatrix& viewProj = (cmd.bOrtho || session.bOrthoMode)
            ? MViewOrtho : MViewPersp;

        FConstants constants = {};
        constants.MVP = cmd.WorldTransform * viewProj;

        
        switch (cmd.Type)
        {     
        case ERenderType::Primitive:
            PrepareShader();
            UpdateMVP(constants.MVP, { 1,1,1 });
            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            switch (cmd.Shape)
            {
            case EPrimitiveShape::Sphere:
                DeviceContext->RSSetState(RasterizerStateOutline);
                RenderPrimitive(vertexBufferSphere, numVerticesSphere);
                break;
            case EPrimitiveShape::Cube:
                DeviceContext->RSSetState(RasterizerState);
                RenderPrimitive(vertexBufferCube, numVerticesCube);
                break;
            case EPrimitiveShape::Triangle:
                DeviceContext->RSSetState(RasterizerState);
                RenderPrimitive(vertexBufferTriangle, numVerticesTriangle);
                break;
            case EPrimitiveShape::Plane:
                DeviceContext->RSSetState(RasterizerState);
                RenderIndexedPrimitive(vertexBufferRect, indexBufferRect, sizeof(rect_indices)/sizeof(UINT));

                break;

            default:
                break;
            }

            break;
        case ERenderType::WorldAxis:
            PrepareShader();
            UpdateMVP(constants.MVP, {1,1,1 });

            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            RenderPrimitive(vertexBufferWorldAxis, numVerticesWorldAxis);

            break;
        case ERenderType::LocalAxis:
            break;
        case ERenderType::Gizmo: {
            PrepareShader();
            DeviceContext->OMSetDepthStencilState(DepthStencilDisable, 1);

            uint32 Red = (cmd.Color >> 24) & 0xFF;
            uint32 Green = (cmd.Color >> 16) & 0xFF;
            uint32 Blue = (cmd.Color >> 8) & 0xFF;
            UpdateMVP(constants.MVP, { Red / 255.f,Green / 255.f, Blue / 255.f });

            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  
            RenderPrimitive(vertexBufferGizmo, numVerticesGizmo);
            DeviceContext->OMSetDepthStencilState(DepthStencilState, 1);
            break;
        }
        case ERenderType::Torus: {
            uint32 Red = (cmd.Color >> 24) & 0xFF;
            uint32 Green = (cmd.Color >> 16) & 0xFF;
            uint32 Blue = (cmd.Color >> 8) & 0xFF;
            UpdateMVP(constants.MVP, { Red / 255.f,Green / 255.f, Blue / 255.f });

            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            PrepareShader();
            RenderPrimitive(vertexBufferTorus, numVerticesTorus);
            break;
        }
        case ERenderType::Highlight:
        {
            FVector direction = session.Camera.Position - FVector(constants.MVP.M[3][0], constants.MVP.M[3][1], constants.MVP.M[3][2]);
            float distance = sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            PrepareOutlineShader();
            UpdateMVP(constants.MVP, 1.0f * distance * 0.01f);
            switch (cmd.Shape)
            {
            case EPrimitiveShape::Sphere:
                DeviceContext->RSSetState(RasterizerState);
                RenderPrimitive(vertexBufferSphere, numVerticesSphere);
                break;
            case EPrimitiveShape::Cube:
                DeviceContext->OMSetDepthStencilState(DepthStencilOutlineState, 1);
                DeviceContext->RSSetState(RasterizerStateOutline);
                RenderPrimitive(vertexBufferCube, numVerticesCube);
                break;
            case EPrimitiveShape::Triangle:
                RenderPrimitive(vertexBufferTriangle, numVerticesTriangle);

                break;
            case EPrimitiveShape::Plane:
                RenderIndexedPrimitive(vertexBufferRect, indexBufferRect, sizeof(rect_indices) / sizeof(UINT));
            default:
                break;
            }
            DeviceContext->OMSetDepthStencilState(DepthStencilState, 1);
            DeviceContext->RSSetState(RasterizerState);

            break;
        }
        case ERenderType::Grid:
        {
            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            
            PrepareShader(GridVertexShader, GridPixelShader, GridInputLayout, ConstantBuffer);
            DeviceContext->VSSetConstantBuffers(1, 1, &GridConstantBuffer);
            DeviceContext->PSSetConstantBuffers(1, 1, &GridConstantBuffer);

            UpdateMVP(constants.MVP, { 1,1,1 });

            UpdateConstantBuffer(session.Camera.Position);
            RenderIndexedPrimitive(vertexBufferGrid, indexBufferGrid, sizeof(UVrect_indices) / sizeof(UINT), StrideUV);
            break;
        }
        default:
            break;
        }
    }
}

void URenderer::EndFrame()
{
    SwapBuffer();
}

void URenderer::UpdateMVP(const FMatrix& mvp, FVector color)
{
    if (ConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

        DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        FConstants* constants = (FConstants*)constantbufferMSR.pData;
        {
            //constants->MVP = mvp;
            constants->MVP = mvp;
            constants->Color = { color.x, color.y, color.z };
        }
        DeviceContext->Unmap(ConstantBuffer, 0);
    }
}

void URenderer::UpdateMVP(const FMatrix& mvp, const float thickness)
{
    if (OutlineConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

        DeviceContext->Map(OutlineConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        FConstants* constants = (FConstants*)constantbufferMSR.pData;
        {
            constants->MVP = mvp;
            constants->thickness = thickness;
            constants->Color = { 1,1,1 };
        }
        DeviceContext->Unmap(OutlineConstantBuffer, 0);
    }
}

void URenderer::UpdateConstantBuffer(const FVector& CameraPos)
{
    if (GridConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

        DeviceContext->Map(GridConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        FcontantsMV* constants = (FcontantsMV*)constantbufferMSR.pData;
        {
            constants->_WorldSpaceCameraPos = CameraPos;
        }
        DeviceContext->Unmap(GridConstantBuffer, 0);
    }
}
