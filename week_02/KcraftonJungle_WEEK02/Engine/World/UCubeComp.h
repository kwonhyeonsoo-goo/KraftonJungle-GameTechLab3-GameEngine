#pragma once
#include "UPrimitiveComponent.h"

class UCubeComp : public UPrimitiveComponent
{
public:
	UCubeComp();

	static UClass* StaticClass();
	UClass* GetClass() const override;
};

