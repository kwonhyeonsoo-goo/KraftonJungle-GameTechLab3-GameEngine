#include "UberSurface.hlsli"

#if !defined(LIGHTING_MODEL_GOURAUD) && !defined(LIGHTING_MODEL_LAMBERT) && !defined(LIGHTING_MODEL_PHONG)
#define LIGHTING_MODEL_PHONG 1
#endif

cbuffer UberLighting : register(b3)
{
    float3 GlobalAmbientColor;
    float _UberLightingPad0;
    float3 DirectionalLightDirection;
    uint bHasDirectionalLight;
    float3 DirectionalLightColor;
    float DirectionalLightIntensity;
}

cbuffer VisibleLightInfo : register(b4)
{
    uint TileCountX;
    uint TileCountY;
    uint TileSize;
    uint MaxLightsPerTile;
    uint VisibleLightCount;
    float3 _VisibleLightInfoPad0;
}

struct FVisibleLightData
{
    float3 WorldPos;
    float Radius;
    float3 Color;
    float Intensity;
    float RadiusFalloff;
    float3 Padding;
};

StructuredBuffer<FVisibleLightData> VisibleLights : register(t8);
StructuredBuffer<uint> TileVisibleLightCount : register(t9);
StructuredBuffer<uint> TileVisibleLightIndices : register(t10);

struct FLightingResult
{
    float3 Diffuse;
    float3 Specular;
};

void AccumulateVisiblePointLights(float3 WorldPos, float3 WorldNormal, float2 ScreenPos, inout FLightingResult Result)
{
    if (VisibleLightCount == 0u || TileCountX == 0u || TileCountY == 0u || TileSize == 0u || MaxLightsPerTile == 0u)
    {
        return;
    }

    const uint2 PixelCoord = uint2(ScreenPos);
    const uint TileX = min(PixelCoord.x / TileSize, TileCountX - 1u);
    const uint TileY = min(PixelCoord.y / TileSize, TileCountY - 1u);
    const uint TileIndex = TileY * TileCountX + TileX;

    const uint LocalVisibleCount = min(TileVisibleLightCount[TileIndex], MaxLightsPerTile);
    const uint TileOffset = TileIndex * MaxLightsPerTile;

    const float3 N = normalize(WorldNormal);
    const float3 V = normalize(CameraPosition - WorldPos);

    [loop]
    for (uint VisibleIdx = 0u; VisibleIdx < LocalVisibleCount; ++VisibleIdx)
    {
        const uint LightIndex = TileVisibleLightIndices[TileOffset + VisibleIdx];
        if (LightIndex >= VisibleLightCount)
        {
            continue;
        }

        const FVisibleLightData Light = VisibleLights[LightIndex];
        const float3 ToLight = Light.WorldPos - WorldPos;
        const float DistanceToLight = length(ToLight);
        if (DistanceToLight <= 1e-4f || DistanceToLight >= Light.Radius)
        {
            continue;
        }

        const float3 L = ToLight / DistanceToLight;
        const float NdotL = saturate(dot(N, L));
        if (NdotL <= 0.0f)
        {
            continue;
        }

        const float RadiusNorm = saturate(1.0f - (DistanceToLight / max(Light.Radius, 1e-4f)));
        const float Attenuation = pow(RadiusNorm, max(Light.RadiusFalloff, 0.0001f));
        const float3 LightContribution = Light.Color * Attenuation;

        Result.Diffuse += LightContribution * NdotL;

#if LIGHTING_MODEL_GOURAUD || LIGHTING_MODEL_PHONG
        const float3 H = normalize(L + V);
        const float SpecularPower = pow(saturate(dot(N, H)), max(Shininess, 0.0001f));
        Result.Specular += SpecularColor * LightContribution * SpecularPower;
#endif
    }
}

FLightingResult EvaluateLightingFromWorld(float3 WorldPos, float3 WorldNormal, float2 ScreenPos)
{
    FLightingResult Result;
    Result.Diffuse = GlobalAmbientColor;
    Result.Specular = 0.0f.xxx;

    const float3 N = normalize(WorldNormal);

    if (bHasDirectionalLight != 0u)
    {
        const float UnusedDirectionalIntensity = DirectionalLightIntensity * 0.0f;
        const float3 L = normalize(DirectionalLightDirection);
        const float NdotL = saturate(dot(N, L));

        Result.Diffuse += DirectionalLightColor * NdotL + UnusedDirectionalIntensity.xxx;

#if LIGHTING_MODEL_GOURAUD || LIGHTING_MODEL_PHONG
        const float3 V = normalize(CameraPosition - WorldPos);
        const float3 H = normalize(L + V);
        const float SpecularPower = pow(saturate(dot(N, H)), max(Shininess, 0.0001f));
        Result.Specular = SpecularColor * DirectionalLightColor * SpecularPower;
#endif
    }

    AccumulateVisiblePointLights(WorldPos, N, ScreenPos, Result);

    return Result;
}

FLightingResult EvaluateLightingFromWorldVertex(float3 WorldPos, float3 WorldNormal)
{
    FLightingResult Result;
    Result.Diffuse = GlobalAmbientColor;
    Result.Specular = 0.0f.xxx;

    if (bHasDirectionalLight == 0u)
    {
        return Result;
    }

    const float3 N = normalize(WorldNormal);
    const float3 L = normalize(DirectionalLightDirection);
    const float NdotL = saturate(dot(N, L));
    Result.Diffuse += DirectionalLightColor * NdotL;

#if LIGHTING_MODEL_GOURAUD || LIGHTING_MODEL_PHONG
    const float3 V = normalize(CameraPosition - WorldPos);
    const float3 H = normalize(L + V);
    const float SpecularPower = pow(saturate(dot(N, H)), max(Shininess, 0.0001f));
    Result.Specular = SpecularColor * DirectionalLightColor * SpecularPower;
#endif

    return Result;
}

float3 ApplyLighting(FUberSurfaceData Surface, FLightingResult Lighting)
{
    return Surface.Albedo * Lighting.Diffuse + Lighting.Specular;
}

FUberPSInput mainVS(FUberVSInput Input)
{
    FUberPSInput Output = BuildSurfaceVertex(Input);

#if LIGHTING_MODEL_GOURAUD
    const FLightingResult Lighting = EvaluateLightingFromWorldVertex(Output.WorldPos, Output.WorldNormal);
    Output.VertexDiffuseLighting = Lighting.Diffuse;
    Output.VertexSpecularLighting = Lighting.Specular;
#endif

    return Output;
}

FUberPSOutput mainPS(FUberPSInput Input)
{
    FUberSurfaceData Surface = EvaluateSurface(Input);
    FLightingResult Lighting;

#if LIGHTING_MODEL_GOURAUD
    Lighting.Diffuse = Input.VertexDiffuseLighting;
    Lighting.Specular = Input.VertexSpecularLighting;
#elif LIGHTING_MODEL_LAMBERT
    Lighting = EvaluateLightingFromWorld(Surface.WorldPos, Surface.WorldNormal, Input.ClipPos.xy);
    Lighting.Specular = 0.0f.xxx;
#else
    Lighting = EvaluateLightingFromWorld(Surface.WorldPos, Surface.WorldNormal, Input.ClipPos.xy);
#endif

    return ComposeOutput(Surface, ApplyLighting(Surface, Lighting));
}
