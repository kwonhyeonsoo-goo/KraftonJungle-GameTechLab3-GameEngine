// --- 상수 버퍼 및 리소스 ---
cbuffer CB_Impostor : register(b0)
{
    matrix ViewProj;
    float3 CameraWorldPos;
    int GridSize; // 16
};

struct InstanceData
{
    matrix WorldMatrix;
    float3 Center;
    float3 Extents;
};
StructuredBuffer<InstanceData> AllInstances : register(t1);

Texture2D AlbedoAtlas : register(t0);
SamplerState LinearSampler : register(s0);

// --- 헬퍼 함수: OctEncode (C++과 동일한 로직) ---
float2 SignNotZero(float2 v)
{
    return float2((v.x >= 0) ? 1 : -1, (v.y >= 0) ? 1 : -1);
}

float2 OctEncode(float3 dir)
{
    float l1norm = abs(dir.x) + abs(dir.y) + abs(dir.z);
    float2 res = dir.xy / l1norm;
    if (dir.z < 0)
    {
        res = (1.0 - abs(res.yx)) * SignNotZero(res);
    }
    return res; // [-1, 1]
}

// --- 쉐이더 본문 ---
struct VS_OUT
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

VS_OUT VSMain(uint VertexID : SV_VertexID, uint InstanceID : SV_InstanceID)
{
    VS_OUT Out;
    InstanceData data = AllInstances[InstanceID];
    
    // 1. 빌보드(항상 카메라를 향하는 사각형) 생성
    // 정점 4개를 VertexID로 생성 (0,1,2,3)
    float2 quadPos = float2(VertexID % 2, VertexID / 2) * 2.0 - 1.0;
    float3 worldPos = data.Center + (quadPos.x * data.Extents.x) + (quadPos.y * data.Extents.y); // 간략화된 빌보드
    
    // 2. 카메라 방향 계산 (임포스터 조각 선택용)
    float3 dir = normalize(data.Center - CameraWorldPos);
    float2 octPos = OctEncode(dir); // [-1, 1]
    
    // 3. 아틀라스 UV 계산
    float2 uv = octPos * 0.5 + 0.5; // [0, 1]
    float2 gridPos = floor(uv * GridSize);
    float2 tileUV = (gridPos + float2(VertexID % 2, 1.0 - (VertexID / 2))) / GridSize;
    
    Out.Pos = mul(float4(worldPos, 1.0), ViewProj);
    Out.UV = tileUV;
    return Out;
}

float4 PSMain(VS_OUT In) : SV_Target
{
    float4 color = AlbedoAtlas.Sample(LinearSampler, In.UV);
    clip(color.a - 0.5); // 투명 영역 컷
    return color;
}