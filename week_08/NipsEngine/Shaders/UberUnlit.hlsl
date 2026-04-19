#include "UberSurface.hlsli"

cbuffer UberLighting : register(b3)
{
    float3 DirectionalLightDirection;
    uint bHasDirectionalLight;
    float3 DirectionalLightColor;
    float DirectionalLightIntensity;
}

FUberPSInput mainVS(FUberVSInput Input)
{
    return BuildSurfaceVertex(Input);
}

FUberPSOutput mainPS(FUberPSInput Input)
{
    FUberSurfaceData Surface = EvaluateSurface(Input);
    const float UnusedDirectionalIntensity = DirectionalLightIntensity * 0.0f;
    return ComposeOutput(Surface, Surface.Albedo + UnusedDirectionalIntensity.xxx);
}
