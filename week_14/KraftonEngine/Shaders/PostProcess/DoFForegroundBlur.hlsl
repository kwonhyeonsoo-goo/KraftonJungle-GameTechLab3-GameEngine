// DoFForegroundBlur.hlsl
// Foreground blur layer from negative signed CoC. Alpha carries the foreground mask.

#include "Common/Functions.hlsli"
#include "Common/SystemResources.hlsli"
#include "Common/SystemSamplers.hlsli"
#include "Common/DoFCommon.hlsli"

PS_Input_UV VS(uint vertexID : SV_VertexID)
{
    return FullscreenTriangleVS(vertexID);
}

float4 AccumulateForeground(float2 uv, float2 offset, float kernelWeight, inout float totalWeight, inout float maxMask)
{
    float2 sampleUV = saturate(uv + offset);
    float sampleCoC = CoCTexture.SampleLevel(LinearClampSampler, sampleUV, 0);
    float fgMask = saturate(-sampleCoC);
    float4 sampleColor = SceneColorTexture.SampleLevel(LinearClampSampler, sampleUV, 0);
    float bokehCandidate = DoFBokehCandidateMask(sampleCoC, sampleColor.rgb);
    float weight = fgMask * kernelWeight * (1.0f - bokehCandidate);
    maxMask = max(maxMask, fgMask);
    totalWeight += weight;
    return sampleColor * weight;
}

float4 PS(PS_Input_UV input) : SV_TARGET
{
    float2 uv = input.uv;
    float4 sharp = SceneColorTexture.SampleLevel(LinearClampSampler, uv, 0);
    float centerCoC = CoCTexture.SampleLevel(LinearClampSampler, uv, 0);
    float centerFgCoC = saturate(-centerCoC);

    float2 texel = DoFGetSceneTexel();
    float radius = max(centerFgCoC, DoFForegroundMinCoCForRadius) * MaxBlurRadius;

    float centerBokehCandidate = DoFBokehCandidateMask(centerCoC, sharp.rgb);
    float totalWeight = centerFgCoC * DoFForegroundCenterWeight * (1.0f - centerBokehCandidate);
    float maxMask = centerFgCoC;
    float4 blur = sharp * totalWeight;

    [unroll]
    for (int i = 0; i < DoFRing0Count; ++i)
    {
        blur += AccumulateForeground(uv, texel * DoFRing0[i] * radius,
            DoFRingSampleWeight(DoFRing0Radius), totalWeight, maxMask);
    }

    if (radius > 2.0f)
    {
        [unroll]
        for (int i = 0; i < DoFRing1Count; ++i)
        {
            blur += AccumulateForeground(uv, texel * DoFRing1[i] * radius,
                DoFRingSampleWeight(DoFRing1Radius), totalWeight, maxMask);
        }
    }

    if (radius > 5.0f)
    {
        [unroll]
        for (int i = 0; i < DoFRing2Count; ++i)
        {
            blur += AccumulateForeground(uv, texel * DoFRing2[i] * radius,
                DoFRingSampleWeight(DoFRing2Radius), totalWeight, maxMask);
        }
    }

    if (radius > 8.0f)
    {
        [unroll]
        for (int i = 0; i < DoFRing3Count; ++i)
        {
            blur += AccumulateForeground(uv, texel * DoFRing3[i] * radius,
                DoFRingSampleWeight(DoFRing3Radius), totalWeight, maxMask);
        }
    }

    float4 color = totalWeight > DoFMinAccumulatedWeight ? blur / max(totalWeight, DoFMinAccumulatedWeight) : sharp;
    if (totalWeight <= DoFMinAccumulatedWeight)
    {
        color.rgb = DoFRemoveBokehEnergyForFallback(centerCoC, color.rgb);
    }
    color.a = saturate(max(maxMask, centerFgCoC));
    return color;
}
