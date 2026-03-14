#include "URenderer.h"
#include "RenderQueue.h"
#include "../Foundation/Math/FMatrix.h"
#include <DirectXMath.h>
#include "../Mesh/Sphere.h"
#include "../Mesh/Cube.h"
#include "../Mesh/Triangle.h"
#include "../Mesh/Rect.h"
#include "../Mesh/Line.h"
#include "../Editor/EditorSession.h"

void URenderer::Create(HWND hWindow)

{
    CreateDeviceAndSwapChain(hWindow);

    CreateFrameBuffer();

    CreateRasterizerState();

    CreatePrimitiveVertexBuffer();


}

void URenderer::RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)

{
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);

    DeviceContext->Draw(numVertices, 0);
}

void URenderer::RenderIndexedPrimitive(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, UINT indexCount)
{
	UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &Stride, &offset);

    DeviceContext->IASetIndexBuffer(indexBuffer,DXGI_FORMAT_R32_UINT, offset);

	DeviceContext->DrawIndexed(indexCount, offset, 0);
}

void URenderer::CreateShader()

{
    ID3DBlob* vertexshaderCSO;
    ID3DBlob* pixelshaderCSO;

    D3DCompileFromFile(L"Shaders/ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexshaderCSO, nullptr);

    Device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

    D3DCompileFromFile(L"Shaders/ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelshaderCSO, nullptr);

    Device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &SimpleInputLayout);

    Stride = sizeof(FVertexSimple);

    vertexshaderCSO->Release();
    pixelshaderCSO->Release();
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

void URenderer::CreateDeviceAndSwapChain(HWND hWindow)
{
    D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

    DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
    swapchaindesc.BufferDesc.Width = 0;
    swapchaindesc.BufferDesc.Height = 0;
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

void URenderer::CreateDepthBuffer(float width, float height)
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
}

void URenderer::CreatePrimitiveVertexBuffer()
{
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
}

void URenderer::ReleasePrimitivVertexBuffer()
{
    ReleaseVertexBuffer(vertexBufferTriangle);
    ReleaseVertexBuffer(vertexBufferCube);
    ReleaseVertexBuffer(vertexBufferSphere);
    ReleaseVertexBuffer(vertexBufferWorldAxis);

}

void URenderer::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC rasterizerdesc = {};
    rasterizerdesc.FillMode = D3D11_FILL_SOLID;
    rasterizerdesc.CullMode = D3D11_CULL_BACK;
    rasterizerdesc.FrontCounterClockwise = false;
    Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
}

void URenderer::ReleaseRasterizerState()
{
    if (RasterizerState)
    {
        RasterizerState->Release();
        RasterizerState = nullptr;
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

void URenderer::ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
{
    
    ReleasePrimitivVertexBuffer();

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
}


#pragma region D3D11 Renderer 함수들

bool URenderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    CreateDeviceAndSwapChain(hwnd);

    CreateFrameBuffer();

    CreateRasterizerState();

    return false;
}

void URenderer::Shutdown()
{
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
    FMatrix mCameraView = session.Camera.GetViewMatrix();

    FVector eye = { 0.0f, 0.0f, -50.0f };   // 카메라 위치
    FVector target = { 0.0f, 0.0f, 0.0f }; // 보는 위치
    FVector up = { 0.0f, 1.0f, 0.0f };     // 위 방향

    float baseHeight = 1024.0f; // 기준 해상도 높이
    float currentHeight = ViewportInfo.Height;

    float fov = DirectX::XMConvertToRadians(60.0f) * (currentHeight / baseHeight);
    float aspect = ViewportInfo.Width / ViewportInfo.Height;   // 창이 정사각형이면 1
    float nearZ = 0.1f;
    float farZ = 100.0f;

    FMatrix mView = FMatrix::LookAt(eye, target, up);


    FMatrix mProjection =
        FMatrix::Perspective(
            fov,
            aspect,
            nearZ,
            farZ
        );

	for (int i = 0; i < queue.GetCommands().size(); ++i)
    {

        const RenderCommand& cmd = queue.GetCommands()[i];
        FConstants constants = {};
        constants.MVP = cmd.WorldTransform * mView * mProjection;

        UpdateMVP(constants.MVP);

        switch (cmd.Type)
        {     
        case ERenderType::Primitive:

            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            switch (cmd.Shape)
            {
            case EPrimitiveShape::Sphere:
                RenderPrimitive(vertexBufferSphere, numVerticesSphere);
                break;
            case EPrimitiveShape::Cube:
                RenderPrimitive(vertexBufferCube, numVerticesCube);
                break;
            case EPrimitiveShape::Triangle:
                RenderPrimitive(vertexBufferTriangle, numVerticesTriangle);

                break;
            case EPrimitiveShape::Plane:
                RenderIndexedPrimitive(vertexBufferRect, indexBufferRect, sizeof(rect_indices)/sizeof(UINT));
            default:
                break;
            }

            break;
        case ERenderType::WorldAxis:
            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            RenderPrimitive(vertexBufferWorldAxis, numVerticesWorldAxis);

            break;
        case ERenderType::LocalAxis:
            break;
        case ERenderType::Gizmo:
            break;
        default:
            break;
        }
    }
}

void URenderer::EndFrame()
{
    SwapBuffer();
}

void URenderer::UpdateMVP(const FMatrix& mvp)
{
    if (ConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

        DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        FConstants* constants = (FConstants*)constantbufferMSR.pData;
        {
            //constants->MVP = mvp;
            constants->MVP = mvp;
        }
        DeviceContext->Unmap(ConstantBuffer, 0);
    }
}
