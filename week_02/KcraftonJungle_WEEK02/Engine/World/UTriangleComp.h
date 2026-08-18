#pragma once
#include "UPrimitiveComponent.h"

class UTriangleComp : public UPrimitiveComponent
{
public:
	UTriangleComp();

	static UClass* StaticClass();
	UClass* GetClass() const override;
};

