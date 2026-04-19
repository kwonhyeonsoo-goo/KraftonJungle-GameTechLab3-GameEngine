#pragma once
#include "RenderPass.h"
#include "Render/Common/ComPtr.h"

struct FLightData;

class FLightCullingPass : public FBaseRenderPass
{
public:
    bool Initialize() override;
    bool Release() override;

private:
    bool Begin(const FRenderPassContext* Context) override;
    bool DrawCommand(const FRenderPassContext* Context) override;
    bool End(const FRenderPassContext* Context) override;

    bool EnsureComputeShader(ID3D11Device* Device);
    bool EnsureInputLightBuffer(ID3D11Device* Device, uint32 RequiredLightCount);
    bool EnsureTileBuffers(ID3D11Device* Device, uint32 RequiredTileCount);
    bool EnsureConstantBuffer(ID3D11Device* Device);

private:
    TComPtr<ID3D11ComputeShader> ComputeShader;
    TComPtr<ID3D11Buffer> LightBuffer;
    TComPtr<ID3D11ShaderResourceView> LightBufferSRV;
    TComPtr<ID3D11Buffer> TileLightCountBuffer;
    TComPtr<ID3D11UnorderedAccessView> TileLightCountUAV;
    TComPtr<ID3D11Buffer> TileLightIndexBuffer;
    TComPtr<ID3D11UnorderedAccessView> TileLightIndexUAV;
    TComPtr<ID3D11Buffer> CullingConstantBuffer;

    uint32 LightBufferCapacity = 0;
    uint32 TileBufferCapacity = 0;
};
