#pragma once

#include "PrimitiveComponent.h"
#include "Math/Vector.h"
#include "Math/Vector4.h"
#include "PrimitiveLineBatch.h"

class ULineBatchComponent : public UPrimitiveComponent
{
	DECLARE_RTTI(ULineBatchComponent, UPrimitiveComponent)

public:
	void Initialize();
	void DrawLine(FVector InStart, FVector InEnd, FVector4 color);
	void DrawCube(FVector InCenter, FQuat InRotation, FVector InScale, FVector4 InColor);
	// 테스트
	void Tick(float deltaTime) override;
};