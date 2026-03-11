Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

cbuffer constants : register(b0)
{
    float3 Offset;
    float Scale;
}

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct VS_INPUT
{
    float4 position : POSITION; // Input position from vertex buffer
    float2 uv : TEXCOORD0; // Input color from vertex buffer
};

float4 PS(PS_INPUT input) : SV_Target
{
    return tex0.Sample(samp0, input.uv);
}

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // output.position = input.position;
    // 위 코드를 아래 코드처럼 수정 하세요.

    input.position.x *= abs(Scale);
    input.position.y *= abs(Scale);
    input.position.z *= abs(Scale);
    
    // Pass the position directly to the pixel shader (no transformation)
    // 상수버퍼를 통해 넘겨 받은 Offset을 더해서 버텍스를 이동 시켜 픽셀쉐이더로 넘김
    output.pos = input.position + float4(Offset, 0.f);


    // Pass the color to the pixel shader
    output.uv.x = lerp(input.uv.x, 1.0f - input.uv.x, Scale < 0);
    output.uv.y = input.uv.y;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    float4 texColor = tex0.Sample(samp0, input.uv);

    clip(texColor.a - 0.5);

    return texColor;
}