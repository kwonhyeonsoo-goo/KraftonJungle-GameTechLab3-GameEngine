#ifndef UBER_SURFACE_INCLUDED
#define UBER_SURFACE_INCLUDED

cbuffer UberFrame : register(b0)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    float3 CameraPosition;
    float _UberFramePad0;
    float bIsWireframe;
    float3 WireframeRGB;
}

cbuffer UberPerObject : register(b1)
{
    row_major float4x4 World;
    row_major float4x4 WorldInverseTranspose;
    float4 PrimitiveColor;
}

cbuffer UberMaterial : register(b2)
{
    float3 AmbientColor;
    float Opacity;

    float3 DiffuseColor;
    float _UberMaterialPad0;

    float3 SpecularColor;
    float Shininess;

    float2 ScrollUV;
    uint bHasDiffuseMap;
    uint bHasSpecularMap;

    float3 EmissiveColor;
    uint bHasAmbientMap;

    uint bHasBumpMap;
    float3 _UberMaterialPad1;
}

Texture2D DiffuseMap : register(t0);
Texture2D AmbientMap : register(t1);
Texture2D SpecularMap : register(t2);
Texture2D BumpMap : register(t3);

SamplerState SampleState : register(s0);

struct FUberVSInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
};

struct FUberPSInput
{
    float4 ClipPos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float3 VertexDiffuseLighting : TEXCOORD3;
    float3 VertexSpecularLighting : TEXCOORD4;
};

struct FUberPSOutput
{
    float4 Color : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 WorldPos : SV_TARGET2;
};

struct FUberSurfaceData
{
    float3 WorldPos;
    float3 WorldNormal;
    float2 UV;
    float4 DiffuseSample;
    float3 Albedo;
    uint bIsEmissive;
};

// 로컬 정점 위치를 World -> View -> Projection 순서로 변환해 최종 클립 공간 좌표를 만든다.
float4 ApplyUberMVP(float3 Position)
{
    float4 WorldPosition = mul(float4(Position, 1.0f), World);
    float4 ViewPosition = mul(WorldPosition, View);
    return mul(ViewPosition, Projection);
}

// 로컬 노멀을 월드 공간 노멀로 변환한다.
// 비균일 스케일에도 올바르게 대응하기 위해 역전치 행렬을 사용한다.
float3 TransformNormalToWorld(float3 Normal)
{
    return normalize(mul(Normal, (float3x3)WorldInverseTranspose));
}

// 기존 머티리얼 슬롯/텍스처 바인딩 계약을 유지하기 위한 보존용 함수다.
// 현재 shading 단계에서는 실제 색 기여를 하지 않지만, 샘플링 경로 자체는 남겨서
// 레거시 파라미터 연결이 끊어지지 않도록 한다.
float3 PreserveLegacyMaterialMaps(float2 UV)
{
    float3 Preserved = 0.0f;

    if (bHasAmbientMap != 0u)
    {
        Preserved += AmbientMap.Sample(SampleState, UV).rgb * 0.0f;
    }

    if (bHasSpecularMap != 0u)
    {
        Preserved += SpecularMap.Sample(SampleState, UV).rgb * 0.0f;
    }

    if (bHasBumpMap != 0u)
    {
        Preserved += BumpMap.Sample(SampleState, UV).rgb * 0.0f;
    }

    return Preserved;
}

// 버텍스 단계에서 픽셀 셰이더가 필요로 하는 surface 입력값을 구성한다.
// 월드 위치, 월드 노멀, 스크롤이 반영된 UV, 클립 공간 좌표를 한 번에 계산한다.
FUberPSInput BuildSurfaceVertex(FUberVSInput Input)
{
    FUberPSInput Output;

    Output.WorldPos = mul(float4(Input.Position, 1.0f), World).xyz;
    Output.WorldNormal = TransformNormalToWorld(Input.Normal);
    Output.UV = Input.UV + ScrollUV;
    Output.ClipPos = ApplyUberMVP(Input.Position);
    Output.VertexDiffuseLighting = 1.0f.xxx;
    Output.VertexSpecularLighting = 0.0f.xxx;

    return Output;
}

// 픽셀 단계에서 사용할 surface 데이터를 평가한다.
// diffuse map 샘플링, alpha clip, albedo 계산, emissive 여부 판정까지 담당한다.
FUberSurfaceData EvaluateSurface(FUberPSInput Input)
{
    FUberSurfaceData Surface;

    Surface.WorldPos = Input.WorldPos;
    Surface.WorldNormal = normalize(Input.WorldNormal);
    Surface.UV = Input.UV;
    Surface.DiffuseSample = float4(1.0f, 1.0f, 1.0f, 1.0f);

    if (bHasDiffuseMap != 0u)
    {
        Surface.DiffuseSample = DiffuseMap.Sample(SampleState, Surface.UV);
        clip(Surface.DiffuseSample.a - 0.001f);
    }

    Surface.DiffuseSample.rgb += PreserveLegacyMaterialMaps(Surface.UV);
    Surface.Albedo = DiffuseColor * Surface.DiffuseSample.rgb;
    Surface.bIsEmissive = any(EmissiveColor > 0.0f) ? 1u : 0u;

    return Surface;
}

// 평가된 surface와 최종 색을 현재 엔진의 MRT 출력 계약에 맞게 패킹한다.
// emissive와 wireframe 같은 특수 표시 정책도 이 단계에서 최종 반영한다.
FUberPSOutput ComposeOutput(FUberSurfaceData Surface, float3 FinalColor)
{
    FUberPSOutput Output;
    float3 ResolvedColor = FinalColor;
    float NormalAlpha = 1.0f;

    if (Surface.bIsEmissive != 0u)
    {
        ResolvedColor = EmissiveColor * Surface.DiffuseSample.rgb;
        NormalAlpha = 2.0f;
    }
    else if (bIsWireframe > 0.5f)
    {
        ResolvedColor = WireframeRGB;
    }

    Output.Color = float4(ResolvedColor, 1.0f);
    Output.Normal = float4(Surface.WorldNormal * 0.5f + 0.5f, NormalAlpha);
    Output.WorldPos = float4(Surface.WorldPos, 1.0f);
    return Output;
}

#endif
