#pragma once
#include "Renderer/ShaderType.h"
#include "Matrix.h"
#include "Transform.h"
#include <cstring>
struct ENGINE_API FCamera
{
	FVector Position = { 0,0,-5 };
	FVector Target = { 0,0,0 };
	FVector Up = { 0, 1,  0 };
	float Fov = 45.0f;
	float AspectRatio = 16.0f / 9.0f;
	float Near = 0.1f;
	float Far = 1000.f;
};

inline void BuildMVP(FConstants& OutCB, const FTransform& T, const FCamera& Cam)
{
	FMatrix World = T.ToMatrix();
	FMatrix View = FMatrix::LookAt(Cam.Position, Cam.Target, Cam.Up);
	FMatrix Proj = FMatrix::Perspective(Cam.Fov, Cam.AspectRatio, Cam.Near, Cam.Far);

	FMatrix MVP = World * View * Proj;
	FMatrix MVP_T = MVP.Transpose();

	memcpy(OutCB.MVP, MVP_T.M, sizeof(OutCB.MVP));
}