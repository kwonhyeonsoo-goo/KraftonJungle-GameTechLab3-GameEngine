#pragma once
#include "../Foundation/Core/CoreTypes.h"
#include "memory"

class UObject;

class UClass {
public:
    using ConstructorFn = std::function<std::unique_ptr<UObject>()>;

    FString ClassName;
    UClass* SuperClass;

    UClass(const FString& name, UClass* super = nullptr)
        : ClassName(name), SuperClass(super), Constructor(nullptr) {
    }

    UClass(const FString& name, UClass* super, ConstructorFn ctor)
        : ClassName(name), SuperClass(super), Constructor(std::move(ctor)) {
    }

    bool IsChildOf(const UClass* other) const;
    bool CanInstantiate() const { return Constructor != nullptr; }

private:
    ConstructorFn Constructor;
	friend class ObjectFactory;
};