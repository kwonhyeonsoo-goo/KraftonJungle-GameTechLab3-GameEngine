#include "ShaderCommon.hlsli"

// Material 상수 버퍼 (b2)
cbuffer MaterialData : register(b2)
{
	float4 BaseColor;
};

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 Result = Input.Color * BaseColor;
	return float4(Result.rgb, 1.0f);
}
