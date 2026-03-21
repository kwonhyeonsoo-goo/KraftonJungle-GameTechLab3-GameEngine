cbuffer FrameConstantBuffer : register(b0)
{
	matrix View;
	matrix Projection;
};

cbuffer ObjectConstantBuffer : register(b1)
{
	matrix World;
};

struct VSInput
{
	float3 Position : POSITION;
	float2 UV : TEXCOORD0;
};

struct PSInput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

PSInput main(VSInput Input)
{
	PSInput Output;

	float4 WorldPos = mul(float4(Input.Position, 1.0f), World);
	float4 ViewPos = mul(WorldPos, View);
	Output.Position = mul(ViewPos, Projection);

	Output.UV = Input.UV;
	return Output;
}