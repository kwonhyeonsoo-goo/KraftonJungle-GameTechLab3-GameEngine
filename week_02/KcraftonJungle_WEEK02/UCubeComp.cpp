#include "UCubeComp.h"
#include "UClass.h"

UCubeComp::UCubeComp() : UPrimitiveComponent(EPrimitiveShape::Cube)
{
}

UClass* UCubeComp::StaticClass()
{
    static UClass ClassInfo("Cube", UPrimitiveComponent::StaticClass());
    return &ClassInfo;
}

UClass* UCubeComp::GetClass() const
{
    return UCubeComp::StaticClass();
}