// DoFComposite.hlsl
// Full-res small-kernel opaque DoF composite.

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

float4 PS(PS_Input_UV input) : SV_TARGET
{
    float2 uv = input.uv;
    float4 sharp = SceneColorTexture.SampleLevel(LinearClampSampler, uv, 0);
    float coc = saturate(CoCTexture.SampleLevel(LinearClampSampler, uv, 0));

    uint width, height;
    SceneColorTexture.GetDimensions(width, height);
    float2 texel = 1.0f / max(float2(width, height), float2(1.0f, 1.0f));
    float radius = coc * MaxBlurRadius;

    float4 blur = sharp * 0.20f;
    blur += SceneColorTexture.SampleLevel(LinearClampSampler, uv + texel * float2( radius,  0.0f), 0) * 0.10f;
    blur += SceneColorTexture.SampleLevel(LinearClampSampler, uv + texel * float2(-radius,  0.0f), 0) * 0.10f;
    blur += SceneColorTexture.SampleLevel(LinearClampSampler, uv + texel * float2( 0.0f,  radius), 0) * 0.10f;
    blur += SceneColorTexture.SampleLevel(LinearClampSampler, uv + texel * float2( 0.0f, -radius), 0) * 0.10f;
    blur += SceneColorTexture.SampleLevel(LinearClampSampler, uv + texel * float2( radius,  radius), 0) * 0.10f;
    blur += SceneColorTexture.SampleLevel(LinearClampSampler, uv + texel * float2(-radius,  radius), 0) * 0.10f;
    blur += SceneColorTexture.SampleLevel(LinearClampSampler, uv + texel * float2( radius, -radius), 0) * 0.10f;
    blur += SceneColorTexture.SampleLevel(LinearClampSampler, uv + texel * float2(-radius, -radius), 0) * 0.10f;

    return lerp(sharp, blur, coc);
}
