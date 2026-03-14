#pragma once
#include "../Foundation/Core/CoreTypes.h"
#include "./UClass.h"

enum class EPrimitiveShape
{
    Cube,
    Sphere,
    Plane,
};


class UObject
{
    uint32 UUID;
    uint32 InternalIndex;
public:

    UObject();
    virtual ~UObject();

    virtual UClass* GetClass() const;
    static UClass* StaticClass();

    virtual EPrimitiveShape GetPrimitiveShape() const = 0 ;

    template<typename T>
    bool IsA() const
    {
        UClass* cls = GetClass();
        return cls != nullptr && cls->IsChildOf(T::StaticClass());
    }

    uint32 GetUUID() const;
    void SetUUID(uint32 uuid);
};