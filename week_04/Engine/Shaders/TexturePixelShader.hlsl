#include "ShaderCommon.hlsli"

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

// Material 상수 버퍼 (b2)
cbuffer MaterialData : register(b2)
{
	float4 BaseColor;
};

cbuffer GlobalData : register(b3)
{
	float Time;
	float2 UVScrollVelocity;
	float Padding;
};

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float2 UV = Input.UV + cos(Time) * UVScrollVelocity;
	float4 Color = Texture.Sample(Sampler, UV);
	return Color;
}
