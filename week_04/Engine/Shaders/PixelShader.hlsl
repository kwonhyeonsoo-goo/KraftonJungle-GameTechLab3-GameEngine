struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
};

float4 main(PS_INPUT Input) : SV_TARGET
{
    // 디렉셔널 라이트 (위-왼쪽-앞에서 비추는 방향)
    //float3 LightDir = normalize(float3(-0.5f, 1.0f, -0.5f));
    //float3 Normal = normalize(Input.Normal);

    // Diffuse (Lambert)
    //float NdotL = saturate(dot(Normal, LightDir));

    // Ambient + Diffuse
    //float3 Ambient = float3(0.15f, 0.15f, 0.2f);
    //float3 Diffuse = Input.Color.rgb * NdotL;

    //float3 FinalColor = Ambient + Diffuse;
	float3 FinalColor = Input.Color.rgb;
	return float4(FinalColor, 1.0f);
}
