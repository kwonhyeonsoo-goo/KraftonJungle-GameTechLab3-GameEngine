#pragma once
#include "PrimitiveComponent.h"

class ENGINE_API USkyComponent : public UPrimitiveComponent
{
	DECLARE_RTTI(USkyComponent, UPrimitiveComponent)

	void Initialize();
};