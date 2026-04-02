#include "ShaderCommon.hlsli"

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float2 UV = frac(Input.UV);
	float2 Grid = abs(frac(Input.UV * 10.0f) - 0.5f);
	float GridLine = step(min(Grid.x, Grid.y), 0.04f);

	float3 BaseColor = float3(UV.x, UV.y, 1.0f - saturate((UV.x + UV.y) * 0.5f));
	float3 GridColor = lerp(BaseColor, float3(1.0f, 1.0f, 1.0f), GridLine);
	return float4(GridColor, 1.0f);
}
