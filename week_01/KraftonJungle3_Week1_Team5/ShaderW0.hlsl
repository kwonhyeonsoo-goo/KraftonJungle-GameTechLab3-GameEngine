cbuffer constants : register(b0)
{
    float3 Offset;
    float Scale;
}

struct VS_INPUT
{
    float4 position : POSITION; // Input position from vertex buffer
    float4 color : COLOR; // Input color from vertex buffer
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float4 color : COLOR; // Color to pass to the pixel shader
};

// SamplerState << 이미지를 어느 방식으로 샘플링 할지 정해주는 변수
// SRV -> 즉 우리가 그려줄 이미지

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // Pass the position directly to the pixel shader (no transformation)
    //output.position = input.position;
    
    // 상수 버퍼를 통해 넘겨받은 Scale값을 곱해서 정점 위치 조절
    input.position.x *= Scale;
    input.position.y *= Scale;
    input.position.z *= Scale;
    
    //상수 버퍼를 통해 넘겨받은 Offset을 더해서 버텍스를 이동시켜 픽셀쉐이더로 옮김
    output.position = input.position + float4(Offset, 0.f);
    
    // Pass the color to the pixel shader
    output.color = input.color;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    return input.color;
}