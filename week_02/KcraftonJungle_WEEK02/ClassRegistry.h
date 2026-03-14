#pragma once
#include "./CoreTypes.h"
#include "UClass.h"

class ClassRegistry
{
public:
	ClassRegistry() {};

    void Register(const FString& typeName, UClass* uclass);
    UClass* Find(const FString& typeName) const;
    bool Contains(const FString& typeName) const;
    TArray<FString> GetAllTypeNames() const;

private:
    TMap<FString, UClass*> Classes;
};

