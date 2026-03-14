#pragma once
#include "USceneComponent.h"
#include "../ObjectKernel/UClass.h"

enum class EPrimitiveShape
{
    Cube,
    Sphere,
    Plane,
    Triangle
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

inline FString PrimitiveShapeToString(EPrimitiveShape shape) {
    switch (shape) {
    case EPrimitiveShape::Cube:     return "Cube";
    case EPrimitiveShape::Sphere:   return "Sphere";
    case EPrimitiveShape::Plane:    return "Plane";
    case EPrimitiveShape::Triangle: return "Triangle";
    default:                        return "Unknown";
    }
}

inline EPrimitiveShape StringToPrimitiveShape(const FString& str) {
    if (str == "Cube")     return EPrimitiveShape::Cube;
    if (str == "Sphere")   return EPrimitiveShape::Sphere;
    if (str == "Plane")    return EPrimitiveShape::Plane;
    if (str == "Triangle") return EPrimitiveShape::Triangle;
    return EPrimitiveShape::Cube; 
}
