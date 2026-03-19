cbuffer constants : register(b0)
{
    row_major float4x4 MVP;
    float3 Color;
    float thickness;
}

cbuffer contantsMV : register(b1)
{
    row_major float4x4 _ObjectToWorld;
    float3 _WorldSpaceCameraPos;
    float padding;

}

#define UNITY_COLORSPACE_GAMMA 1

struct VS_INPUT
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float2 uv : TEXCOORD; // Color to pass to the pixel shader
};


float3 GammaToLinearSpace(float3 col)
{
    return pow(col, 2.2);
}

float3 LinearToGammaSpace(float3 col)
{
    return pow(col, 1.0 / 2.2);
}

PS_INPUT mainVS(VS_INPUT input)
{
    float _GridScale = 1.0;

    PS_INPUT output;
    
    output.position = mul(input.position, MVP);
    
    float3 worldPos = mul(float4(input.position.xyz, 1), _ObjectToWorld);
    float3 cameraCenteringOffset = floor(_WorldSpaceCameraPos * _GridScale);
    float3 cameraSnappedWorldPos = worldPos.xyz * _GridScale - cameraCenteringOffset;

    output.uv = cameraSnappedWorldPos.xy;

    return output;

}

float PristineGrid(float2 uv, float2 lineWidth)
{

    lineWidth = saturate(lineWidth);
    float4 uvDDXY = float4(ddx(uv), ddy(uv));
    float2 uvDeriv = float2(length(uvDDXY.xz), length(uvDDXY.yw));
    bool2 invertLine = lineWidth > 0.5;
    float2 targetWidth = invertLine ? 1.0 - lineWidth : lineWidth;
    float2 drawWidth = clamp(targetWidth, uvDeriv, 1);
    float2 lineAA = max(uvDeriv, 0.001) * 1.5;
    float2 gridUV = abs(frac(uv) * 2.0 - 1.0);
    gridUV = invertLine ? gridUV : 1.0 - gridUV;
    float2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= saturate(targetWidth / drawWidth);
    grid2 = lerp(grid2, targetWidth, saturate(uvDeriv * 2.0 - 1.0));
    grid2 = invertLine ? 1.0 - grid2 : grid2;
    return lerp(grid2.x, 1.0, grid2.y);
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float _LineWidthX = 0.01;
    float _LineWidthY = 0.01;
    
    float4 _LineColor = float4(1, 1, 1, 1);
    float4 _BaseColor = float4(0, 0, 0, 0);
    
    float grid = PristineGrid(input.uv, float2(_LineWidthX, _LineWidthY));

    return lerp(_BaseColor, _LineColor, grid * _LineColor.a);


}

