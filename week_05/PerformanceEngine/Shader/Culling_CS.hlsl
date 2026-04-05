struct InstanceData
{
    float4x4 WorldMatrix;
    float3 Center;
    float padding1;
    float3 Extents;
    float padding2;
};

StructuredBuffer<InstanceData> AllInstances : register(t0);
Texture2D<float> HiZPyramid : register(t1);
StructuredBuffer<uint> LastFrameVisibility : register(t2);

RWStructuredBuffer<uint> VisibilityResults : register(u0);
SamplerState PointSampler : register(s0);

cbuffer CullingParams : register(b0)
{
    row_major float4x4 ViewProjection;
    float2 RenderTargetSize;
    uint PrimitiveCount;
    float Padding;
};

void GetScreenRect(float3 center, float3 extents, float4x4 viewProj, out float4 rect, out float minZ, out bool intersectsNearPlane)
{
    float3 v[8];
    v[0] = center + extents * float3(-1, -1, -1);
    v[1] = center + extents * float3(1, -1, -1);
    v[2] = center + extents * float3(-1, 1, -1);
    v[3] = center + extents * float3(1, 1, -1);
    v[4] = center + extents * float3(-1, -1, 1);
    v[5] = center + extents * float3(1, -1, 1);
    v[6] = center + extents * float3(-1, 1, 1);
    v[7] = center + extents * float3(1, 1, 1);

    float2 minXY = float2(1, 1);
    float2 maxXY = float2(-1, -1);
    minZ = 1.0f;
    intersectsNearPlane = false;

    [unroll]
    for (int i = 0; i < 8; i++)
    {
        float4 projPos = mul(float4(v[i], 1.0f), viewProj);
        if (projPos.w <= 0.0001f)
        {
            intersectsNearPlane = true;
            continue;
        }
        
        float3 ndc = projPos.xyz / projPos.w;
        float2 uv;
        uv.x = ndc.x * 0.5f + 0.5f;
        uv.y = 1.0f - (ndc.y * 0.5f + 0.5f);
        
        minXY = min(minXY, uv);
        maxXY = max(maxXY, uv);
        minZ = min(minZ, ndc.z);
    }

    // 영역을 아주 약간 확장하여 보수적으로 판단 (0.005 = 약 5픽셀)
    rect.xy = saturate(minXY - 0.005f);
    rect.zw = saturate(maxXY + 0.005f);
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.x;
    if (idx >= PrimitiveCount) 
        return;

    InstanceData data = AllInstances[idx];
    
    float4 aabbRect;
    float minZ;
    bool bIntersectsNearPlane;
    GetScreenRect(data.Center, data.Extents, ViewProjection, aabbRect, minZ, bIntersectsNearPlane);
    
    if (bIntersectsNearPlane)
    {
        VisibilityResults[idx] = 3;
        return;
    }

    // 화면 밖 컬링
    if (aabbRect.z < aabbRect.x || aabbRect.w < aabbRect.y)
    {
        VisibilityResults[idx] = 0;
        return;
    }

    // ceil을 사용하여 더 보수적인(상위) Mip 선택
    float2 size = (aabbRect.zw - aabbRect.xy) * 1024.0f;
    float mip = clamp(ceil(log2(max(size.x, size.y))), 0.0f, 10.0f);

    float4 depthSamples;
    depthSamples.x = HiZPyramid.SampleLevel(PointSampler, aabbRect.xy, mip).r;
    depthSamples.y = HiZPyramid.SampleLevel(PointSampler, aabbRect.zy, mip).r;
    depthSamples.z = HiZPyramid.SampleLevel(PointSampler, aabbRect.xw, mip).r;
    depthSamples.w = HiZPyramid.SampleLevel(PointSampler, aabbRect.zw, mip).r;

    float maxHizDepth = max(max(depthSamples.x, depthSamples.y), max(depthSamples.z, depthSamples.w));
    
    // Bias를 0.01f로 늘려 컬링을 완화
    bool isVisibleNow = (minZ <= maxHizDepth);
    
    uint lastCount = LastFrameVisibility[idx];
    
    if (isVisibleNow)
    {
        VisibilityResults[idx] = 3;
    }
    else
    {
        VisibilityResults[idx] = (lastCount > 0) ? lastCount - 1 : 0;
    }
}
