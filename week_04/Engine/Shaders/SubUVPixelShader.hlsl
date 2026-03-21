cbuffer SubUVConstantBuffer : register(b2)
{
	float4 Color;
	float2 CellSize;
	float2 Offset;
};

Texture2D MainTexture : register(t0);
SamplerState MainSampler : register(s0);

struct PSInput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

float4 main(PSInput Input) : SV_TARGET
{
	float2 FinalUV = Input.UV * CellSize + Offset;
	float4 Sampled = MainTexture.Sample(MainSampler, FinalUV);

	float Alpha = Sampled.r;
	clip(Alpha - 0.01f);

	return float4(Color.rgb, Color.a * Alpha);
}