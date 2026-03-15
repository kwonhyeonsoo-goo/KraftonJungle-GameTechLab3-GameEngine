// ShaderW0.hlsl
cbuffer constants : register(b0)
{
    row_major float4x4 MVP;
    float3 Color;
    float thickness;
}


struct VS_INPUT
{
    float4 position : POSITION;
    float4 color : COLOR; 
    float3 normal : NORMAL;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};


PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    output.color = input.color; 

    float3 expand = input.position.xyz + input.normal.xyz * thickness;
    
    output.position = mul(float4(expand, 1.0), MVP);

    return output;
}


float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    return float4(0.969f, 0.404f, 0.027f, 1.0f);
    //return float4(Color, 1);

}
