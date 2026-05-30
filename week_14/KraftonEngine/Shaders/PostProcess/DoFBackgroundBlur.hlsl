// DoFBackgroundBlur.hlsl
// Depth-aware far/background blur from positive signed CoC.

#include "Common/Functions.hlsli"
#include "Common/SystemResources.hlsli"
#include "Common/SystemSamplers.hlsli"

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

float ViewDistanceFromDepth(float2 uv, float depth)
{
    if (depth <= 0.0f)
    {
        return FocusDistance + max(FocusRange, 1.0f);
    }

    float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
    float4 clipPos = float4(ndc, depth, 1.0f);
    float4 worldH = mul(clipPos, InvViewProj);
    float3 worldPos = worldH.xyz / max(worldH.w, 1.0e-6f);
    return length(worldPos - CameraWorldPos);
}

float4 AccumulateBackground(float2 uv, float2 offset, float currentDistance, float kernelWeight, inout float totalWeight)
{
    float2 sampleUV = saturate(uv + offset);
    float sampleCoC = CoCTexture.SampleLevel(LinearClampSampler, sampleUV, 0);
    float sampleDepth = SceneDepthTexture.SampleLevel(PointClampSampler, sampleUV, 0);
    float sampleDistance = ViewDistanceFromDepth(sampleUV, sampleDepth);

    float bgWeight = saturate(sampleCoC);
    float nearerReject = sampleDistance + 1.0f < currentDistance ? 0.15f : 1.0f;
    float weight = bgWeight * nearerReject * kernelWeight;
    totalWeight += weight;
    return SceneColorTexture.SampleLevel(LinearClampSampler, sampleUV, 0) * weight;
}

float4 PS(PS_Input_UV input) : SV_TARGET
{
    float2 uv = input.uv;
    float4 sharp = SceneColorTexture.SampleLevel(LinearClampSampler, uv, 0);
    float centerCoC = CoCTexture.SampleLevel(LinearClampSampler, uv, 0);
    float centerBgCoC = saturate(centerCoC);

    uint width, height;
    SceneColorTexture.GetDimensions(width, height);
    float2 texel = 1.0f / max(float2(width, height), float2(1.0f, 1.0f));
    float radius = max(centerBgCoC, 0.15f) * MaxBlurRadius;

    float centerDepth = SceneDepthTexture.SampleLevel(PointClampSampler, uv, 0);
    float centerDistance = ViewDistanceFromDepth(uv, centerDepth);

    float totalWeight = 0.20f * centerBgCoC;
    float4 blur = sharp * totalWeight;

    blur += AccumulateBackground(uv, texel * float2( radius,  0.0f), centerDistance, 0.90f, totalWeight);
    blur += AccumulateBackground(uv, texel * float2(-radius,  0.0f), centerDistance, 0.90f, totalWeight);
    blur += AccumulateBackground(uv, texel * float2( 0.0f,  radius), centerDistance, 0.90f, totalWeight);
    blur += AccumulateBackground(uv, texel * float2( 0.0f, -radius), centerDistance, 0.90f, totalWeight);
    blur += AccumulateBackground(uv, texel * float2( radius,  radius), centerDistance, 0.65f, totalWeight);
    blur += AccumulateBackground(uv, texel * float2(-radius,  radius), centerDistance, 0.65f, totalWeight);
    blur += AccumulateBackground(uv, texel * float2( radius, -radius), centerDistance, 0.65f, totalWeight);
    blur += AccumulateBackground(uv, texel * float2(-radius, -radius), centerDistance, 0.65f, totalWeight);

    float safeWeight = max(totalWeight, 1.0e-4f);
    return totalWeight > 1.0e-4f ? blur / safeWeight : sharp;
}
