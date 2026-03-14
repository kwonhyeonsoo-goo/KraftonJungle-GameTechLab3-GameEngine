cbuffer Constants : register(b0)
{
	float4x4 WVP;
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
	Output.Position = mul(float4(Input.Position, 1.0f), WVP);
	Output.Color = Input.Color;
	Output.Normal = mul(Input.Normal, (float3x3) World);
	return Output;
}
