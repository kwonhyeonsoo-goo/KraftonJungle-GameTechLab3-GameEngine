#pragma once
#include "./CoreTypes.h"

class UClass {
public:
    FString ClassName;
    UClass* SuperClass;

    UClass(const FString& name, UClass* super = nullptr);

    bool IsChildOf(const UClass* other) const;
};