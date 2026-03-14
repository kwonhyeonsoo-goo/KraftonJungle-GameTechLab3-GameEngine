#pragma once
#include "UPrimitiveComponent.h"

class USphereComp : public UPrimitiveComponent
{
public:
    USphereComp();

    static UClass* StaticClass();
    UClass* GetClass() const override;
};