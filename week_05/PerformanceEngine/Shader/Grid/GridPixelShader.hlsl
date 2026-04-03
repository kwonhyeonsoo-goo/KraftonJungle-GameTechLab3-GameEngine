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

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
};

float ComputeGridLine(float2 WorldPlane, float CellSize)
{
    float2 Coordinate = WorldPlane / CellSize;
    float2 Derivative = max(fwidth(Coordinate), float2(1e-5f, 1e-5f));
    float2 Grid = abs(frac(Coordinate - 0.5f) - 0.5f) / Derivative;
    return 1.0f - saturate(min(Grid.x, Grid.y));
}

float ComputeAxisLine(float Value)
{
    float Width = max(fwidth(Value), 1e-5f);
    return 1.0f - saturate(abs(Value) / Width);
}

float4 PSMain(VSOutput Input) : SV_TARGET
{
    float2 WorldPlane = Input.WorldPos.xy;

    float MinorLine = ComputeGridLine(WorldPlane, MinorCellSize);
    float MajorLine = ComputeGridLine(WorldPlane, MajorCellSize);
    float XAxisLine = ComputeAxisLine(Input.WorldPos.y);
    float YAxisLine = ComputeAxisLine(Input.WorldPos.x);

    float3 Color = float3(0.0f, 0.0f, 0.0f);
    float Alpha = 0.0f;

    Color = lerp(Color, MinorLineColor.rgb, MinorLine * MinorLineColor.a);
    Alpha = max(Alpha, MinorLine * MinorLineColor.a);

    Color = lerp(Color, MajorLineColor.rgb, MajorLine * MajorLineColor.a);
    Alpha = max(Alpha, MajorLine * MajorLineColor.a);

    Color = lerp(Color, XAxisColor.rgb, XAxisLine * XAxisColor.a);
    Alpha = max(Alpha, XAxisLine * XAxisColor.a);

    Color = lerp(Color, YAxisColor.rgb, YAxisLine * YAxisColor.a);
    Alpha = max(Alpha, YAxisLine * YAxisColor.a);

    float DistanceFromCamera = distance(CameraPosition.xy, WorldPlane);
    float Fade = saturate(1.0f - DistanceFromCamera / FadeDistance);
    Alpha *= Fade;

    if (Alpha <= 0.001f)
    {
        discard;
    }

    return float4(Color, Alpha);
}
