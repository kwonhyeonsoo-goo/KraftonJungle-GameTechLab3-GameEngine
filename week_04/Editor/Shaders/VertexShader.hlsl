cbuffer Constants : register(b0)
{
    row_major float4x4 MVP;
    float4 HighlightColor;
    uint bIsSelected;
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color    : COLOR;
    float3 Normal   : NORMAL;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
};

VS_OUTPUT main(VS_INPUT Input)
{
    VS_OUTPUT Output;
    Output.Position = mul(float4(Input.Position, 1.0f), MVP);
    Output.Color = bIsSelected ? HighlightColor : Input.Color;
    return Output;
}
