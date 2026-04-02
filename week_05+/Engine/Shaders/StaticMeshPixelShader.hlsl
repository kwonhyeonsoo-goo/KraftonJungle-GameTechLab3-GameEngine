#include "ShaderCommon.hlsli"

Texture2D g_txColor : register(t0);
SamplerState g_Sample : register(s0);

cbuffer MaterialData : register(b2)
{
	float4 ColorTint;
	float UVScrollEnabled;
	float2 UVScrollSpeed;
	float UVScrollPadding;
};

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	//TotalTime을 사용해 UV을 옮깁니다.
	float2 FinalUV = Input.UV;
	if (UVScrollEnabled > 0.5f)
	{
		FinalUV += UVScrollSpeed * TotalTime;
		//frac이 소수 부분만 남기므로, 샘플링에 쓰는 값은 늘 0~1 범위가 됩니다.
		FinalUV = frac(FinalUV);
	}

	float4 TexColor = g_txColor.Sample(g_Sample, FinalUV);
	return TexColor * Input.Color * ColorTint;
}
