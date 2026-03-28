#pragma once
#include "Component/PrimitiveComponent.h"

class UMeshComponent : public UPrimitiveComponent
{
public:
	DECLARE_RTTI(UMeshComponent, UPrimitiveComponent)
	// 빈 껍데기 (향후 skeletal mesh 확장용)
};
