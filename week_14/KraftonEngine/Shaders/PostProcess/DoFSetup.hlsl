// DoFSetup.hlsl
// Depth -> signed circle of confusion.

#include "Common/Functions.hlsli"
#include "Common/SystemResources.hlsli"

cbuffer DoFBuffer : register(b2)
{
    float FocusDistance;
    float FocusRange;
    float MaxBlurRadius;
    float _Pad;
};

PS_Input_UV VS(uint vertexID : SV_VertexID)
{
    return FullscreenTriangleVS(vertexID);
}

float PS(PS_Input_UV input) : SV_TARGET
{
    int2 coord = int2(input.position.xy);
    float depth = SceneDepthTexture.Load(int3(coord, 0));

    if (depth <= 0.0f)
    {
        return 1.0f;
    }

    float2 ndc = float2(input.uv.x * 2.0f - 1.0f, 1.0f - input.uv.y * 2.0f);
    float4 clipPos = float4(ndc, depth, 1.0f);
    float4 worldH = mul(clipPos, InvViewProj);
    float3 worldPos = worldH.xyz / max(worldH.w, 1.0e-6f);

    float viewDistance = length(worldPos - CameraWorldPos);
    float safeRange = max(FocusRange, 1.0f);
    return clamp((viewDistance - FocusDistance) / safeRange, -1.0f, 1.0f);
}
