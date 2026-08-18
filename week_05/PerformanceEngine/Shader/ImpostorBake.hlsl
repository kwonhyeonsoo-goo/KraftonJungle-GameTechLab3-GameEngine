// --- 상수 버퍼 (Constant Buffer) ---
cbuffer CB_BakeMatrix : register(b0)
{
    matrix WorldViewProj;
    matrix World;
};

// --- 입력/출력 구조체 ---
struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
    float3 WorldNormal : NORMAL;
};

struct PS_OUTPUT
{
    float4 Albedo : SV_Target0; // 첫 번째 타겟: 색상 + 투명도
    float4 NormalDepth : SV_Target1; // 두 번째 타겟: 월드 노말 + 깊이값
};

// --- 텍스처 및 샘플러 ---
Texture2D DiffuseMap : register(t0);
SamplerState Sampler : register(s0);

// ==========================================
// 버텍스 쉐이더 (Vertex Shader)
// ==========================================
PS_INPUT VS_Main(VS_INPUT input)
{
    PS_INPUT output;
    
    // 위치를 직교 투영(Orthographic) 카메라 화면으로 변환
    output.Pos = mul(float4(input.Pos, 1.0f), WorldViewProj);
    output.UV = input.UV;
    
    // 노말을 월드 공간으로 회전시킴
    output.WorldNormal = mul(input.Normal, (float3x3) World);
    
    return output;
}

// ==========================================
// 픽셀 쉐이더 (Pixel Shader)
// ==========================================
PS_OUTPUT PS_Main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    // 1. 색상 샘플링 및 투명도 컷오프(알파 테스트)
    float4 baseColor = DiffuseMap.Sample(Sampler, input.UV);
    clip(baseColor.a - 0.33f); // 투명한 픽셀은 그리지 않고 버림(Discard)
    
    output.Albedo = baseColor;
    
    // 2. 노말 및 깊이(Depth) 계산
    float3 normal = normalize(input.WorldNormal);
    
    // 노말 범위 [-1, 1]을 [0, 1]로 압축 (텍스처에 예쁘게 저장하기 위함)
    float3 encodedNormal = normal * 0.5f + 0.5f;
    
    // 직교 투영(Orthographic)이기 때문에 Pos.z가 이미 선형적인 [0, 1] 깊이값을 가짐
    float depth = input.Pos.z;
    
    output.NormalDepth = float4(encodedNormal, depth);
    
    return output;
}