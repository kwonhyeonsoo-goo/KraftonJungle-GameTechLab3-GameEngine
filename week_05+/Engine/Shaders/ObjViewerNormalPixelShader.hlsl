#include "ShaderCommon.hlsli"

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float3 NormalColor = normalize(Input.Normal) * 0.5f + 0.5f;
	return float4(NormalColor, 1.0f);
}
