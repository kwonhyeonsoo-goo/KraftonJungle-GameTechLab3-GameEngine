#pragma once
#include "./UObject.h"
#include "../Foundation/Core/CoreTypes.h"


class ObjectStore {
public:
    ObjectStore();

    void Add(UObject* obj);
    void Remove(uint32 uuid);
    void Clear();
    UObject* Find(uint32 uuid) const;

    const TArray<UObject*>& GUObjectArray() const;

    uint32 Count() const;

private:
    TArray<UObject*> Objects;
};