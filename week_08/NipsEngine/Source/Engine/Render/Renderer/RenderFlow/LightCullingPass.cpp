#include "LightCullingPass.h"
#include "Core/Paths.h"
#include "Render/Scene/RenderBus.h"
#include "Render/Scene/RenderCommand.h"
#include <cstring>
#include "UI/EditorConsoleWidget.h"

namespace
{
    constexpr uint32 LightCullingTileSize = 16;
    constexpr uint32 MaxLightsPerTile = 64;
    constexpr uint32 ThreadGroupSizeX = 8;
    constexpr uint32 ThreadGroupSizeY = 8;

    struct FLightCullingConstants
    {
        FMatrix View;
        FMatrix Projection;
        uint32 LightCount = 0;
        uint32 TileCountX = 0;
        uint32 TileCountY = 0;
        uint32 TileSize = 0;
        float ViewportWidth = 0.0f;
        float ViewportHeight = 0.0f;
        float Padding[2] = { 0.0f, 0.0f };
    };

    uint32 CeilDivide(uint32 Numerator, uint32 Denominator)
    {
        return (Numerator + Denominator - 1u) / Denominator;
    }
}

bool FLightCullingPass::Initialize()
{
    return true;
}

bool FLightCullingPass::Release()
{
    ComputeShader.Reset();
    LightBuffer.Reset();
    LightBufferSRV.Reset();
    TileLightCountBuffer.Reset();
    TileLightCountUAV.Reset();
    TileLightIndexBuffer.Reset();
    TileLightIndexUAV.Reset();
    CullingConstantBuffer.Reset();
    LightBufferCapacity = 0;
    TileBufferCapacity = 0;
    return true;
}

bool FLightCullingPass::Begin(const FRenderPassContext* Context)
{
    if (!Context || !Context->Device || !Context->DeviceContext || !Context->RenderBus || !Context->RenderTargets)
    {
        return false;
    }

    OutSRV = PrevPassSRV;
    OutRTV = PrevPassRTV;
    return true;
}

bool FLightCullingPass::DrawCommand(const FRenderPassContext* Context)
{
    if (!EnsureComputeShader(Context->Device) || !EnsureConstantBuffer(Context->Device))
    {
        return false;
    }

    const float Width = Context->RenderTargets->Width;
    const float Height = Context->RenderTargets->Height;
    if (Width <= 0.0f || Height <= 0.0f)
    {
        return true;
    }

    const uint32 TileCountX = CeilDivide(static_cast<uint32>(Width), LightCullingTileSize);
    const uint32 TileCountY = CeilDivide(static_cast<uint32>(Height), LightCullingTileSize);
    const uint32 TileCount = TileCountX * TileCountY;
    if (TileCount == 0)
    {
        return true;
    }

    if (!EnsureTileBuffers(Context->Device, TileCount))
    {
        return false;
    }

    const TArray<FLightData>& Lights = Context->RenderBus->GetLight();
    const uint32 LightCount = static_cast<uint32>(Lights.size());

    if (LightCount > 0)
    {
        if (!EnsureInputLightBuffer(Context->Device, LightCount))
        {
            return false;
        }

        D3D11_MAPPED_SUBRESOURCE MappedLightBuffer = {};
        if (FAILED(Context->DeviceContext->Map(LightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedLightBuffer)))
        {
            return false;
        }
        std::memcpy(MappedLightBuffer.pData, Lights.data(), sizeof(FLightData) * LightCount);
        Context->DeviceContext->Unmap(LightBuffer.Get(), 0);
    }

    FLightCullingConstants Constants = {};
    Constants.View = Context->RenderBus->GetView();
    Constants.Projection = Context->RenderBus->GetProj();
    Constants.LightCount = LightCount;
    Constants.TileCountX = TileCountX;
    Constants.TileCountY = TileCountY;
    Constants.TileSize = LightCullingTileSize;
    Constants.ViewportWidth = Width;
    Constants.ViewportHeight = Height;

    D3D11_MAPPED_SUBRESOURCE MappedCB = {};
    if (FAILED(Context->DeviceContext->Map(CullingConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedCB)))
    {
        return false;
    }
    std::memcpy(MappedCB.pData, &Constants, sizeof(Constants));
    Context->DeviceContext->Unmap(CullingConstantBuffer.Get(), 0);

    ID3D11ComputeShader* CS = ComputeShader.Get();
    Context->DeviceContext->CSSetShader(CS, nullptr, 0);

    ID3D11Buffer* CBuffers[] = { CullingConstantBuffer.Get() };
    Context->DeviceContext->CSSetConstantBuffers(0, 1, CBuffers);

    ID3D11ShaderResourceView* SRVs[] = { LightCount > 0 ? LightBufferSRV.Get() : nullptr };
    Context->DeviceContext->CSSetShaderResources(0, 1, SRVs);

    UINT ClearValues[4] = { 0u, 0u, 0u, 0u };
    Context->DeviceContext->ClearUnorderedAccessViewUint(TileLightCountUAV.Get(), ClearValues);
    Context->DeviceContext->ClearUnorderedAccessViewUint(TileLightIndexUAV.Get(), ClearValues);

    ID3D11UnorderedAccessView* UAVs[] = { TileLightCountUAV.Get(), TileLightIndexUAV.Get() };
    Context->DeviceContext->CSSetUnorderedAccessViews(0, 2, UAVs, nullptr);

    const uint32 DispatchX = CeilDivide(TileCountX, ThreadGroupSizeX);
    const uint32 DispatchY = CeilDivide(TileCountY, ThreadGroupSizeY);
    Context->DeviceContext->Dispatch(DispatchX, DispatchY, 1);

    return true;
}

bool FLightCullingPass::End(const FRenderPassContext* Context)
{
    ID3D11ShaderResourceView* NullSRVs[] = { nullptr };
    Context->DeviceContext->CSSetShaderResources(0, 1, NullSRVs);

    ID3D11UnorderedAccessView* NullUAVs[] = { nullptr, nullptr };
    Context->DeviceContext->CSSetUnorderedAccessViews(0, 2, NullUAVs, nullptr);

    ID3D11Buffer* NullCBs[] = { nullptr };
    Context->DeviceContext->CSSetConstantBuffers(0, 1, NullCBs);
    Context->DeviceContext->CSSetShader(nullptr, nullptr, 0);
    return true;
}

bool FLightCullingPass::EnsureComputeShader(ID3D11Device* Device)
{
    if (ComputeShader)
    {
        return true;
    }

    TComPtr<ID3DBlob> CSBlob;
    TComPtr<ID3DBlob> ErrorBlob;
    const HRESULT CompileResult = D3DCompileFromFile(
        FPaths::ToWide("Shaders/Multipass/LightCullingCS.hlsl").c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "mainCS",
        "cs_5_0",
        0,
        0,
        CSBlob.GetAddressOf(),
        ErrorBlob.GetAddressOf());

    if (FAILED(CompileResult))
    {
        if (ErrorBlob)
        {
            UE_LOG("LightCulling CS Compile Error: %s", static_cast<const char*>(ErrorBlob->GetBufferPointer()));
        }
        else
        {
            UE_LOG("Failed to compile LightCullingCS.hlsl");
        }
        return false;
    }

    const HRESULT CreateResult =
        Device->CreateComputeShader(CSBlob->GetBufferPointer(), CSBlob->GetBufferSize(), nullptr, ComputeShader.GetAddressOf());
    if (FAILED(CreateResult))
    {
        UE_LOG("Failed to create LightCulling compute shader");
        return false;
    }

    return true;
}

bool FLightCullingPass::EnsureInputLightBuffer(ID3D11Device* Device, uint32 RequiredLightCount)
{
    if (RequiredLightCount <= LightBufferCapacity && LightBuffer && LightBufferSRV)
    {
        return true;
    }

    const uint32 NewCapacity = RequiredLightCount;

    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.ByteWidth = static_cast<uint32>(sizeof(FLightData) * NewCapacity);
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = sizeof(FLightData);

    TComPtr<ID3D11Buffer> NewBuffer;
    if (FAILED(Device->CreateBuffer(&BufferDesc, nullptr, NewBuffer.GetAddressOf())))
    {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SRVDesc.Buffer.FirstElement = 0;
    SRVDesc.Buffer.NumElements = NewCapacity;

    TComPtr<ID3D11ShaderResourceView> NewSRV;
    if (FAILED(Device->CreateShaderResourceView(NewBuffer.Get(), &SRVDesc, NewSRV.GetAddressOf())))
    {
        return false;
    }

    LightBuffer = std::move(NewBuffer);
    LightBufferSRV = std::move(NewSRV);
    LightBufferCapacity = NewCapacity;
    return true;
}

bool FLightCullingPass::EnsureTileBuffers(ID3D11Device* Device, uint32 RequiredTileCount)
{
    if (RequiredTileCount <= TileBufferCapacity && TileLightCountBuffer && TileLightCountUAV && TileLightIndexBuffer && TileLightIndexUAV)
    {
        return true;
    }

    const uint32 NewTileCount = RequiredTileCount;
    const uint32 TileLightIndexCount = NewTileCount * MaxLightsPerTile;

    D3D11_BUFFER_DESC CountBufferDesc = {};
    CountBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    CountBufferDesc.ByteWidth = static_cast<uint32>(sizeof(uint32) * NewTileCount);
    CountBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    CountBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    CountBufferDesc.StructureByteStride = sizeof(uint32);

    TComPtr<ID3D11Buffer> NewCountBuffer;
    if (FAILED(Device->CreateBuffer(&CountBufferDesc, nullptr, NewCountBuffer.GetAddressOf())))
    {
        return false;
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC CountUAVDesc = {};
    CountUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    CountUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    CountUAVDesc.Buffer.FirstElement = 0;
    CountUAVDesc.Buffer.NumElements = NewTileCount;

    TComPtr<ID3D11UnorderedAccessView> NewCountUAV;
    if (FAILED(Device->CreateUnorderedAccessView(NewCountBuffer.Get(), &CountUAVDesc, NewCountUAV.GetAddressOf())))
    {
        return false;
    }

    D3D11_BUFFER_DESC IndexBufferDesc = {};
    IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    IndexBufferDesc.ByteWidth = static_cast<uint32>(sizeof(uint32) * TileLightIndexCount);
    IndexBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    IndexBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    IndexBufferDesc.StructureByteStride = sizeof(uint32);

    TComPtr<ID3D11Buffer> NewIndexBuffer;
    if (FAILED(Device->CreateBuffer(&IndexBufferDesc, nullptr, NewIndexBuffer.GetAddressOf())))
    {
        return false;
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC IndexUAVDesc = {};
    IndexUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    IndexUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    IndexUAVDesc.Buffer.FirstElement = 0;
    IndexUAVDesc.Buffer.NumElements = TileLightIndexCount;

    TComPtr<ID3D11UnorderedAccessView> NewIndexUAV;
    if (FAILED(Device->CreateUnorderedAccessView(NewIndexBuffer.Get(), &IndexUAVDesc, NewIndexUAV.GetAddressOf())))
    {
        return false;
    }

    TileLightCountBuffer = std::move(NewCountBuffer);
    TileLightCountUAV = std::move(NewCountUAV);
    TileLightIndexBuffer = std::move(NewIndexBuffer);
    TileLightIndexUAV = std::move(NewIndexUAV);
    TileBufferCapacity = NewTileCount;
    return true;
}

bool FLightCullingPass::EnsureConstantBuffer(ID3D11Device* Device)
{
    if (CullingConstantBuffer)
    {
        return true;
    }

    D3D11_BUFFER_DESC CBDesc = {};
    CBDesc.ByteWidth = sizeof(FLightCullingConstants);
    CBDesc.Usage = D3D11_USAGE_DYNAMIC;
    CBDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    CBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    return SUCCEEDED(Device->CreateBuffer(&CBDesc, nullptr, CullingConstantBuffer.GetAddressOf()));
}
