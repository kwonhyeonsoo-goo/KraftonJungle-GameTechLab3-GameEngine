#pragma once
#include "../Foundation/Core/CoreTypes.h"
#include "./UClass.h"

class UObject
{
public:
    uint32 UUID;
    uint32 InternalIndex;

    UObject();
    virtual ~UObject();

    virtual UClass* GetClass() const;
    static UClass* StaticClass();

    template<typename T>
    bool IsA() const
    {
        UClass* cls = GetClass();
        return cls != nullptr && cls->IsChildOf(T::StaticClass());
    }

    uint32 GetUUID() const;
    void SetUUID(uint32 uuid);
};