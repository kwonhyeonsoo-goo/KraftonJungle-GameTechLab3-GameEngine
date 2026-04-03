cbuffer FGridConstants : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 View;
    row_major float4x4 Projection;

    float4 CameraPosition;
    float4 MinorLineColor;
    float4 MajorLineColor;
    float4 XAxisColor;
    float4 YAxisColor;

    float MinorCellSize;
    float MajorCellSize;
    float FadeDistance;
    float Padding0;
};

struct VSInput
{
    float3 Position : POSITION;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
};

VSOutput VSMain(VSInput Input)
{
    VSOutput Output;

    float4 WorldPosition = mul(float4(Input.Position, 1.0f), World);
    float4 ViewPosition = mul(WorldPosition, View);
    Output.Position = mul(ViewPosition, Projection);
    Output.WorldPos = WorldPosition.xyz;

    return Output;
}
