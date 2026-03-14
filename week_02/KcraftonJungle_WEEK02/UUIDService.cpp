#include "UUIDService.h"

UUIDService::UUIDService() : NextUUID(1)
{
}

uint32 UUIDService::GenUUID()
{
    return NextUUID++;
}

void UUIDService::SyncNextUUID(uint32 value)
{
    NextUUID = value;
}

uint32 UUIDService::GetNext() const
{
    return NextUUID;
}