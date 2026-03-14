#include "UObject.h"

UObject::UObject() : UUID(0), InternalIndex(0)
{
}

UObject::~UObject()
{
}

UClass* UObject::GetClass() const
{
    return UObject::StaticClass();
}

UClass* UObject::StaticClass()
{
    static UClass ClassInfo("UObject", nullptr);
    return &ClassInfo;
}

uint32 UObject::GetUUID() const
{
    return UUID;
}

void UObject::SetUUID(uint32 uuid)
{
    UUID = uuid;
}