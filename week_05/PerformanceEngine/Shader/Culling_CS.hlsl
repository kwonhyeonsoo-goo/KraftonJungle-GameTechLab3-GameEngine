struct InstanceData
{
    row_major float4x4 WorldMatrix;
    float3 Center;
    float padding1;
    float3 Extents;
    float padding2;
};

StructuredBuffer<InstanceData> AllInstances : register(t0);
Texture2D<float> HiZPyramid : register(t1);
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
    // AABB의 8개 정점 생성
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
        // View-Projection 변환
        float4 projPos = mul(float4(v[i], 1.0f), viewProj);
        
        if (projPos.w <= 0.0001f)
        {
            intersectsNearPlane = true;
            continue;
        }
        
        // NDC 좌표로 변환 (W로 나누기)
        float3 ndc = projPos.xyz / projPos.w;
        
        float2 uv;
        uv.x = ndc.x * 0.5f + 0.5f;
        uv.y = 1.0f - (ndc.y * 0.5f + 0.5f);
        
        minXY = min(minXY, uv);
        maxXY = max(maxXY, uv);
        
        // 가장 가까운 깊이값 저장 (컬링 비교용)
        minZ = min(minZ, ndc.z);
    }

    // 결과: x,y는 좌상단 / z,w는 우하단 (0~1 범위로 변환)
    rect.xy = minXY;
    rect.zw = maxXY;
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
        VisibilityResults[idx] = 1;
        return;
    }

    float width = (aabbRect.z - aabbRect.x) * RenderTargetSize.x;
    float height = (aabbRect.w - aabbRect.y) * RenderTargetSize.y;
    float mip = clamp(floor(log2(max(width, height))) - 1.0f, 0.0f, 10.0f);
    
    float4 depthSamples;
    depthSamples.x = HiZPyramid.SampleLevel(PointSampler, aabbRect.xy, mip).r;
    depthSamples.y = HiZPyramid.SampleLevel(PointSampler, aabbRect.zy, mip).r;
    depthSamples.z = HiZPyramid.SampleLevel(PointSampler, aabbRect.xw, mip).r;
    depthSamples.w = HiZPyramid.SampleLevel(PointSampler, aabbRect.zw, mip).r;
    
    float maxHizDepth = max(max(depthSamples.x, depthSamples.y), max(depthSamples.z, depthSamples.w));
    
    if (minZ <= maxHizDepth + 0.0001f)
    {
        VisibilityResults[idx] = 1;
    }
    else
    {
        VisibilityResults[idx] = 0;
    }
}