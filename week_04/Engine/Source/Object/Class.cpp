#include "Class.h"

UClass::UClass(FString InName, UClass* InSuperClass, CreateFunc InCreateFunc)
	: Name(std::move(InName)),
	SuperClass(InSuperClass),
	Factory(InCreateFunc)
{
}

const FString& UClass::GetName() const
{
	return Name;
}

UClass* UClass::GetSuperClass() const
{
	return SuperClass;
}

bool UClass::IsChildOf(const UClass* Other) const
{
    for (const UClass* Current = this; Current != nullptr; Current = Current->SuperClass)
    {
        if (Current == Other)
        {
            return true;
        }
    }
    return false;
}

UObject* UClass::CreateInstance(UObject* InOuter, const FString& InName) const
{
    return Factory ? Factory(InOuter, InName) : nullptr;
}
