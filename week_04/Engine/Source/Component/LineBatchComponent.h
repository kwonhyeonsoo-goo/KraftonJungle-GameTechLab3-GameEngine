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

	// 미구현
	void DrawCube(FVector InCenter, FQuat InRotation, FVector InScale, FVector4 color) = delete;

};