
Texture2D FontTexture : register(t0);
SamplerState FontSampler : register(s0);

cbuffer TextData : register(b2)
{
	float4 TextColor;
};

struct PSInput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

float4 main(PSInput Input) : SV_TARGET
{
	float4 SampledColor = FontTexture.Sample(FontSampler, Input.UV);

	float Alpha = SampledColor.r; // 또는 g, b
	clip(Alpha - 0.01f);

	return float4(TextColor.rgb, TextColor.a * Alpha);
	//return SampledColor;
}