#pragma once
#include "UPrimitiveComponent.h"

class USphereComp : public UPrimitiveComponent
{
public:
    USphereComp();

    static UClass* StaticClass();
    UClass* GetClass() const override;

	const float GetRadius() const { return Radius; }
    void SetRadius(float radius) { Radius = radius; }
private:
	float Radius = 1.0f;
};