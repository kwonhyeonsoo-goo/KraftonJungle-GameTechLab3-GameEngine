#include "ShaderCommon.hlsli"

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	float3 WorldPos : POSITION1;
	uint ObjID : OBJECTID;
};

uint main(PS_INPUT Input) : SV_TARGET
{
	return Input.ObjID;
}