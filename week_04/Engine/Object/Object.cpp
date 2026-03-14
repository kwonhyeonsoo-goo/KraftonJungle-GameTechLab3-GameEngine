#include "Object/Object.h"

int32 UObject::GetTotalBytes()
{
    return UObject::TotalAllocationBytes;
}

int32 UObject::GetTotalCounts()
{
    return UObject::TotalAllocationCounts;
}



UObject::UObject(uint32 InUUID, size_t InObjectType) :UUID(InUUID), ObjectType(InObjectType) 
{

}