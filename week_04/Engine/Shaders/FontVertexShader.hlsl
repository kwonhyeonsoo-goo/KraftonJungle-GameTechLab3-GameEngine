
cbuffer FrameData : register(b0)
{
	float4x4 View;
	float4x4 Projection;
};

cbuffer ObjectData : register(b1)
{
	float4x4 World;
};

struct VSInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

VSOutput main(VSInput Input)
{
	VSOutput Output;
	
	float4 WorldPos = mul(float4(Input.Position, 1.0f), World);
	float4 ViewPos = mul(WorldPos, View);
	Output.Position = mul(ViewPos, Projection);
	Output.UV = Input.TexCoord;
	
	return Output;
}