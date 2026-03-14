struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
};

float4 main(PS_INPUT Input) : SV_TARGET
{
	return float4(1.0f, 0.5f, 0.0f, 1.0f); // 주황색 아웃라인
}
