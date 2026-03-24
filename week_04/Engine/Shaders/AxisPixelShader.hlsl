cbuffer CameraBuffer : register(b2)
{
	float3 CameraPos;
	float GridSize;
	float LineThickness;
	float3 padding;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION; // 클립 공간 좌표
	float3 WorldPos : TEXCOORD0; // 월드 공간 좌표
	float3 LocalPos : TEXCOORD1;
	int PlaneNo : TEXCOORD2;
};

// ==========================================
// Pixel Shader
// ==========================================
float4 main(VS_OUTPUT input) : SV_Target
{
	float dist = distance(input.WorldPos, CameraPos);
	if (dist < 0.5)
		discard;
    
	float maxDistance = 1000.0f;
	float range = 1000.0f;
	if (input.PlaneNo == 0)
	{
        
		float2 derivative = fwidth(input.LocalPos.xy) * (range / GridSize);
		float2 coord = input.WorldPos.xy / GridSize;
    
		float2 grid = abs(frac(coord - 0.5f) - 0.5f) / max(derivative, 0.0001f);
		float lineAlpha = clamp(LineThickness - min(grid.x, grid.y), 0.0f, 1.0f);

		float2 axisDrawWidth = derivative * GridSize * 1.5f;
		axisDrawWidth = clamp(axisDrawWidth, 0.01f, 5.0f);
		float axisX = 1.0f - smoothstep(0.0f, axisDrawWidth.x + 0.001f, abs(input.WorldPos.x));
		float axisY = 1.0f - smoothstep(0.0f, axisDrawWidth.y + 0.001f, abs(input.WorldPos.y));
        
		float3 color = float3(0.5f, 0.5f, 0.5f);
		color = lerp(color, float3(0.2f, 1.0f, 0.2f), axisX); // Green (Y축 방향 선)
		color = lerp(color, float3(1.0f, 0.2f, 0.2f), axisY); // Red (X축 방향 선)

        
		float fade = pow(saturate(1.0f - dist / maxDistance), 2.0f);
		float lodFade = saturate(0.8f - max(derivative.x, derivative.y));
		float finalAlpha = max(lineAlpha, max(axisX, axisY)) * fade * lodFade;

		if (finalAlpha < 0.01f)
			discard;
		return float4(color, finalAlpha);
	}
	else
	{
		float derivative = 0.0f;
		float WorldPos = 0.0f;
        
		if (input.PlaneNo == 1)
		{
			derivative = fwidth(input.WorldPos.y);
			WorldPos = input.WorldPos.y;
		}
		else
		{
			derivative = fwidth(input.WorldPos.x);
			WorldPos = input.WorldPos.x;
		}

		float axisDrawWidth = derivative * 1.5f;
		float axisZ_Line = 1.0f - smoothstep(0.0f, axisDrawWidth + 0.0f, abs(WorldPos));

		float3 color = float3(0.2f, 0.2f, 1.0f); // Z축 표준 색상: Blue
    
		float fade = pow(saturate(1.0f - dist / maxDistance), 2.0f);
		float finalAlpha = axisZ_Line * fade;

		if (finalAlpha < 0.01f)
			discard;

		return float4(color, finalAlpha);
	}
}
