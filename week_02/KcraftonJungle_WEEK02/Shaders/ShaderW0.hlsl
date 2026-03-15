// ShaderW0.hlsl

// ShaderW0.hlsl �ҽ� ���� ������ �Ʒ� ������� ������ �߰� �ϼ���.
cbuffer constants : register(b0)
{
    row_major float4x4 MVP;
    float thickness;
    float3 padding;
}


struct VS_INPUT
{
    float4 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float4 color : COLOR; // Color to pass to the pixel shader
};

//DirectX에서는 'World * View * Projection' 
//HLSL에서는  'Projection * View * World'로 계산

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    output.color = input.color; 

    output.position = mul(input.position, MVP);

    return output;
}


float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    return input.color;
}
