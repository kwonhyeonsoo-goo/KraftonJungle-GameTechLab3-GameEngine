#pragma once
#include "PrimitiveComponent.h"

class ENGINE_API UObjComponent : public UPrimitiveComponent
{
public:
	DECLARE_RTTI(UObjComponent, UPrimitiveComponent)
	void Initialize();

	void LoadPrimitive(const FString& FilePath);
};
