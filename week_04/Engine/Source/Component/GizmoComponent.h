#pragma once
#include "PrimitiveComponent.h"

class ENGINE_API UGizmoComponent : public UPrimitiveComponent
{
public:
	DECLARE_RTTI(UGizmoComponent, UPrimitiveComponent)

	void Initialize();
};

