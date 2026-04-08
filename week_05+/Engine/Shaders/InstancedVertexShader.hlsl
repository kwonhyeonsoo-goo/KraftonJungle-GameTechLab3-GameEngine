// InstancedVertexShader.hlsl

cbuffer FrameBuffer : register(b0)
{
	matrix View;
	matrix Projection;
	float TotalTime;
	float3 Padding;
};

// b1 (ObjectConstantBuffer)은 사용하지 않습니다.

struct VS_INPUT
{
    // 슬롯 0
	float3 Pos : POSITION;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
    
    // 슬롯 1 (인스턴스 단위 월드 행렬)
	float4 World0 : WORLD0;
	float4 World1 : WORLD1;
	float4 World2 : WORLD2;
	float4 World3 : WORLD3;
	uint ObjID : BLENDINDICES0;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	float3 WorldPos : POSITION1;
	uint ObjID : BLENDINDICES0;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
    
    // 4개의 벡터를 조합하여 월드 행렬 완성
	matrix instanceWorld = matrix(
        input.World0,
        input.World1,
        input.World2,
        input.World3
    );

    // 정점 위치에 인스턴스 월드 행렬 적용
	float4 worldPos = mul(float4(input.Pos, 1.0f), instanceWorld);
	output.WorldPos = worldPos.xyz;
    
	output.Pos = mul(worldPos, View);
	output.Pos = mul(output.Pos, Projection);
    
	output.Color = input.Color;
    // 노멀 행렬 변환 (Uniform Scale 가정)
	output.Normal = normalize(mul(input.Normal, (float3x3) instanceWorld));
	output.UV = input.UV;
	output.ObjID = input.ObjID;
	return output;
}