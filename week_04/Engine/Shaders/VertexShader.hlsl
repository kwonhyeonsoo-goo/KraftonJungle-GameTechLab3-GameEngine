// b0: 프레임당 1회 (카메라)
cbuffer FrameData : register(b0)
{
	float4x4 View;
	float4x4 Projection;
};

// b1: 오브젝트당
cbuffer ObjectData : register(b1)
{
	float4x4 World;
};

struct VS_INPUT
{
	float3 Position : POSITION;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
};

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;
	float4 WorldPos = mul(float4(Input.Position, 1.0f), World);
	float4 ViewPos = mul(WorldPos, View);
	Output.Position = mul(ViewPos, Projection);
	Output.Color = Input.Color;
	Output.Normal = mul(Input.Normal, (float3x3) World);
	return Output;
}
