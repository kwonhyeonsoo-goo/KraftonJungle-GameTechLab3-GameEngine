#pragma once
#include "USceneComponent.h"
#include "../ObjectKernel/UClass.h"

enum class EPrimitiveShape
{
    Cube,
    Sphere,
    Plane,
};

class UPrimitiveComponent : public USceneComponent
{
public:
    EPrimitiveShape Shape;
    bool bVisible = true;

    FVector BoundsMin;
    FVector BoundsMax;

    UPrimitiveComponent();
    explicit UPrimitiveComponent(EPrimitiveShape shape);

    static UClass* StaticClass();
    UClass* GetClass() const override;
};