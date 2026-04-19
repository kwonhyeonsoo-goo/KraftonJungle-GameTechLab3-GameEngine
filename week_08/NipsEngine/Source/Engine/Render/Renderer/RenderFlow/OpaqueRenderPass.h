#pragma once
#include "RenderPass.h"
#include "Render/Common/ComPtr.h"

class FOpaqueRenderPass : public FBaseRenderPass
{
public:
    bool Initialize() override;
    bool Release() override;

protected:
    bool Begin(const FRenderPassContext* Context) override;
    bool DrawCommand(const FRenderPassContext* Context) override;
    bool End(const FRenderPassContext* Context) override;

private:
    bool EnsureLightCullingConstantBuffer(ID3D11Device* Device);
    void BindLightCullingResources(const FRenderPassContext* Context);

private:
    struct FVisibleLightConstants
    {
        uint32 TileCountX = 0;
        uint32 TileCountY = 0;
        uint32 TileSize = 0;
        uint32 MaxLightsPerTile = 0;
        uint32 LightCount = 0;
        float Padding[3] = { 0.0f, 0.0f, 0.0f };
    };

    TComPtr<ID3D11Buffer> VisibleLightConstantBuffer;
};
