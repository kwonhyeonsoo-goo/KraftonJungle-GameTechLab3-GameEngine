#pragma once
#include "UPrimitiveComponent.h"

class UPlaneComp : public UPrimitiveComponent
{
public:
    UPlaneComp();

    static UClass* StaticClass();
    UClass* GetClass() const override;
};