// DoFForegroundBlur.hlsl
// Foreground blur layer from negative signed CoC. Alpha carries the foreground mask.

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

float4 AccumulateForeground(float2 uv, float2 offset, float kernelWeight, inout float totalWeight, inout float maxMask)
{
    float2 sampleUV = saturate(uv + offset);
    float sampleCoC = CoCTexture.SampleLevel(LinearClampSampler, sampleUV, 0);
    float fgMask = saturate(-sampleCoC);
    float weight = fgMask * kernelWeight;
    maxMask = max(maxMask, fgMask);
    totalWeight += weight;
    return SceneColorTexture.SampleLevel(LinearClampSampler, sampleUV, 0) * weight;
}

float4 PS(PS_Input_UV input) : SV_TARGET
{
    float2 uv = input.uv;
    float4 sharp = SceneColorTexture.SampleLevel(LinearClampSampler, uv, 0);
    float centerCoC = CoCTexture.SampleLevel(LinearClampSampler, uv, 0);
    float centerFgCoC = saturate(-centerCoC);

    uint width, height;
    SceneColorTexture.GetDimensions(width, height);
    float2 texel = 1.0f / max(float2(width, height), float2(1.0f, 1.0f));
    float radius = max(centerFgCoC, 0.20f) * MaxBlurRadius;

    float totalWeight = centerFgCoC * 0.25f;
    float maxMask = centerFgCoC;
    float4 blur = sharp * totalWeight;

    blur += AccumulateForeground(uv, texel * float2( radius,  0.0f), 1.0f, totalWeight, maxMask);
    blur += AccumulateForeground(uv, texel * float2(-radius,  0.0f), 1.0f, totalWeight, maxMask);
    blur += AccumulateForeground(uv, texel * float2( 0.0f,  radius), 1.0f, totalWeight, maxMask);
    blur += AccumulateForeground(uv, texel * float2( 0.0f, -radius), 1.0f, totalWeight, maxMask);
    blur += AccumulateForeground(uv, texel * float2( radius,  radius), 0.75f, totalWeight, maxMask);
    blur += AccumulateForeground(uv, texel * float2(-radius,  radius), 0.75f, totalWeight, maxMask);
    blur += AccumulateForeground(uv, texel * float2( radius, -radius), 0.75f, totalWeight, maxMask);
    blur += AccumulateForeground(uv, texel * float2(-radius, -radius), 0.75f, totalWeight, maxMask);

    float4 color = totalWeight > 1.0e-4f ? blur / max(totalWeight, 1.0e-4f) : sharp;
    color.a = saturate(max(maxMask, centerFgCoC));
    return color;
}
