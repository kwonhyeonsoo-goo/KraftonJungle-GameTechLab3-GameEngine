#include "UClass.h"

UClass::UClass(const FString& name, UClass* super)
{
	ClassName = name;
	SuperClass = super;
}

bool UClass::IsChildOf(const UClass* other) const
{
    if (other == nullptr) return false;

    const UClass* current = this;
    while (current != nullptr)
    {
        if (current == other)
            return true;

        current = current->SuperClass;
    }

    return false;
}


