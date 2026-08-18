#include "UClass.h"

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


