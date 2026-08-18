struct InstanceData
{
    float4x4 WorldMatrix;
    float3 Center;
    float Padding1;
    float3 Extents;
    float Padding2;
};

Texture2D AlbedoAtlas : register(t0);
StructuredBuffer<InstanceData> AllInstances : register(t1);
StructuredBuffer<uint> ImpostorIndices : register(t2);
SamplerState LinearSampler : register(s0);

cbuffer FrameCB : register(b0)
{
    row_major float4x4 ViewProj;
    float2 RTSize;
    uint PrimCount;
    float Padding;
    float4 CameraPos;
};

struct VS_OUT
{
    float4 Pos : SV_POSITION;
    float2 LocalUV : TEXCOORD0;
    float2 GridPosF : TEXCOORD1;
};

float2 SignNotZero(float2 v)
{
    return float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}
float2 OctEncode(float3 dir)
{
    float l1norm = abs(dir.x) + abs(dir.y) + abs(dir.z);
    float2 res = dir.xy / l1norm;
    if (dir.z < 0.0)
        res = (1.0 - abs(res.yx)) * SignNotZero(res);
    return res;
}

VS_OUT VSMain(uint VertexID : SV_VertexID, uint InstanceID : SV_InstanceID)
{
    VS_OUT Out;
    uint RealIndex = ImpostorIndices[InstanceID];
    InstanceData data = AllInstances[RealIndex];

    // 1. 완벽한 월드 중심점 (스파이크 붕괴 방지)
    float3 worldCenter = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), data.WorldMatrix).xyz;

    // 2. 카메라를 향하는 절대 방향 (원근 왜곡 방지)
    float3 dirToCameraWorld = normalize(CameraPos.xyz - worldCenter);
    
    // 3. 메쉬 회전에 맞춘 로컬 방향
    float3 dirToCameraLocal = normalize(mul((float3x3) data.WorldMatrix, dirToCameraWorld));

    // 4. 구형 빌보드 축 생성
    float3 upGuide = float3(0, 1, 0);
    if (abs(dirToCameraWorld.y) > 0.999f)
    {
        upGuide = float3(0, 0, 1);
    }
    float3 right = normalize(cross(upGuide, dirToCameraWorld));
    float3 up = normalize(cross(dirToCameraWorld, right));

    float2 quadPos;
    quadPos.x = (VertexID % 2) ? 1.0 : -1.0;
    quadPos.y = (VertexID / 2) ? -1.0 : 1.0;

    Out.LocalUV.x = (VertexID % 2) ? 1.0 : 0.0;
    Out.LocalUV.y = (VertexID / 2) ? 1.0 : 0.0;

    // 5. 타일 좌표 계산
    float2 octPos = OctEncode(dirToCameraLocal);
    float2 atlasUV = octPos * 0.5 + 0.5;
    float2 gridPosF = atlasUV * 16.0;
    gridPosF.y = 16.0 - gridPosF.y;
    Out.GridPosF = gridPosF;

    // 6. 스케일 적용
    float radius = max(data.Extents.x, max(data.Extents.y, data.Extents.z));
    float scale = radius * 2.0f;
    if (scale < 0.1f)
        scale = 2.0f;

    // 7. 최종 빌보드 월드 위치 적용 후 투영
    float3 finalWorldPos = worldCenter + (right * quadPos.x * scale) + (up * quadPos.y * scale);
    Out.Pos = mul(float4(finalWorldPos, 1.0), ViewProj);
    
    return Out;
}

float4 SampleTile(float2 gridCell, float2 localUV)
{
    float2 g = clamp(gridCell, 0.0, 15.0);
    float2 safeUV = clamp(localUV, 0.02, 0.98);
    float2 uv = (g + safeUV) / 16.0;
    return AlbedoAtlas.Sample(LinearSampler, uv);
}

float4 PSMain(VS_OUT In) : SV_Target
{
    float2 gf = In.GridPosF - 0.5;
    float2 g00 = floor(gf);
    float2 blend = frac(gf);

    float4 c00 = SampleTile(g00, In.LocalUV);
    float4 c10 = SampleTile(g00 + float2(1.0, 0.0), In.LocalUV);
    float4 c01 = SampleTile(g00 + float2(0.0, 1.0), In.LocalUV);
    float4 c11 = SampleTile(g00 + float2(1.0, 1.0), In.LocalUV);

    float4 color = lerp(lerp(c00, c10, blend.x), lerp(c01, c11, blend.x), blend.y);

    // 검은 테두리(Halo) 완화: 알파값으로 나누어 원래 색상 복원
    if (color.a > 0.001f)
    {
        color.rgb /= color.a;
    }

    clip(color.a - 0.5f);
    
    // 밝기 톤 복원
    color.rgb = pow(abs(color.rgb), 1.0 / 2.2);

    return color;
}