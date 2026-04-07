#include "ShaderCommon.hlsli"

uint main(VS_OUTPUT Input) : SV_TARGET
{
	return Input.ObjID;
}