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

float4 AccumulateForeground(float2 uv, float2 offset, float distancePixels, float kernelWeight, inout float totalWeight, inout float totalCoverage, inout float totalKernelWeight)
{
    float2 sampleUV = saturate(uv + offset);
    float sampleCoC = CoCTexture.SampleLevel(LinearClampSampler, sampleUV, 0);
    float fgMask = saturate(-sampleCoC);
    float sampleRadius = max(fgMask, DoFForegroundMinCoCForRadius) * MaxBlurRadius;
    float apertureCoverage = smoothstep(-DoFForegroundGatherFeather, DoFForegroundGatherFeather, sampleRadius - distancePixels);
    float4 sampleColor = SceneColorTexture.SampleLevel(LinearClampSampler, sampleUV, 0);
    float bokehCandidate = DoFBokehCandidateMask(sampleCoC, sampleColor.rgb);
    float weight = fgMask * apertureCoverage * kernelWeight * (1.0f - bokehCandidate);
    totalCoverage += fgMask * apertureCoverage * kernelWeight;
    totalKernelWeight += kernelWeight;
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
    float radius = MaxBlurRadius;

    float centerBokehCandidate = DoFBokehCandidateMask(centerCoC, sharp.rgb);
    float totalWeight = centerFgCoC * DoFForegroundCenterWeight * (1.0f - centerBokehCandidate);
    float totalCoverage = centerFgCoC * DoFForegroundCenterWeight;
    float totalKernelWeight = DoFForegroundCenterWeight;
    float4 blur = sharp * totalWeight;

    [unroll]
    for (int i = 0; i < DoFRing0Count; ++i)
    {
        blur += AccumulateForeground(uv, texel * DoFRing0[i] * radius,
            DoFRing0Radius * radius, DoFRingSampleWeight(DoFRing0Radius), totalWeight, totalCoverage, totalKernelWeight);
    }

    if (radius > 2.0f)
    {
        [unroll]
        for (int i = 0; i < DoFRing1Count; ++i)
        {
            blur += AccumulateForeground(uv, texel * DoFRing1[i] * radius,
                DoFRing1Radius * radius, DoFRingSampleWeight(DoFRing1Radius), totalWeight, totalCoverage, totalKernelWeight);
        }
    }

    if (radius > 5.0f)
    {
        [unroll]
        for (int i = 0; i < DoFRing2Count; ++i)
        {
            blur += AccumulateForeground(uv, texel * DoFRing2[i] * radius,
                DoFRing2Radius * radius, DoFRingSampleWeight(DoFRing2Radius), totalWeight, totalCoverage, totalKernelWeight);
        }
    }

    if (radius > 8.0f)
    {
        [unroll]
        for (int i = 0; i < DoFRing3Count; ++i)
        {
            blur += AccumulateForeground(uv, texel * DoFRing3[i] * radius,
                DoFRing3Radius * radius, DoFRingSampleWeight(DoFRing3Radius), totalWeight, totalCoverage, totalKernelWeight);
        }
    }

    float4 color = totalWeight > DoFMinAccumulatedWeight ? blur / max(totalWeight, DoFMinAccumulatedWeight) : sharp;
    if (totalWeight <= DoFMinAccumulatedWeight)
    {
        color.rgb = DoFRemoveBokehEnergyForFallback(centerCoC, color.rgb);
    }
    color.a = saturate(totalCoverage / max(totalKernelWeight, DoFMinAccumulatedWeight));
    return color;
}
