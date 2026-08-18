#pragma once
#include "./UObject.h"
#include "../Foundation/Core/CoreTypes.h"
#include <memory>


class ObjectStore {
public:
    ObjectStore();

    void Add(std::unique_ptr<UObject> obj);
    void Remove(uint32 uuid);
    void Clear();
    void ReleaseMemory();
    void ClearAndReleaseMemory();
    UObject* Find(uint32 uuid) const;

    const TArray<std::unique_ptr<UObject>>& GUObjectArray() const;

    uint32 Count() const;

private:
    TArray<std::unique_ptr<UObject>> Objects;
    TMap<uint32, UObject*>           Index;
};