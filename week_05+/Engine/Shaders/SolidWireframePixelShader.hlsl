#include "ShaderCommon.hlsli"

cbuffer MaterialData : register(b2)
{
	float4 FillColor;
	float4 LineColor;
	float EdgeThickness;
	float3 MaterialPadding;
};

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float3 Barycentric = saturate(Input.Color.rgb);
	float3 EdgeWidth = fwidth(Barycentric) * max(EdgeThickness, 0.5f);
	float3 Smoothed = smoothstep(0.0f.xxx, EdgeWidth, Barycentric);
	float InteriorFactor = min(Smoothed.x, min(Smoothed.y, Smoothed.z));

	return lerp(LineColor, FillColor, InteriorFactor);
}
