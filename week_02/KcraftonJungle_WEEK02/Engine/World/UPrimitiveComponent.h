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

    UPrimitiveComponent();
    explicit UPrimitiveComponent(EPrimitiveShape shape);

    static UClass* StaticClass();
    UClass* GetClass() const override;

    FVector GetBoundMin() const;
    void SetBoundMin(FVector min);

    FVector GetBoundMax() const;
    void SetBoundMax(FVector max);

protected:
    FVector BoundsMin;
    FVector BoundsMax;
};