/* Constant Buffers */

cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 MVP;
};

cbuffer FontColor : register(b1)
{
    float4 FontColor; // RGBA
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

PS_INPUT VS_Font(VS_INPUT input)
{
    PS_INPUT output;
    output.Position = mul(float4(input.Position, 1.0f), MVP);
    output.TexCoord = input.TexCoord;
    return output;
}

Texture2D FontAtlas : register(t0);
SamplerState FontSampler : register(s0);

float4 PS_Font(PS_INPUT input) : SV_TARGET
{
    float4 sample = FontAtlas.Sample(FontSampler, input.TexCoord);
    sample.a = sample.r;
    return sample;
}
