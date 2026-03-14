#include "Object/Object.h"

int32 UObject::GetTotalBytes()
{
    return UObject::TotalAllocationBytes;
}

int32 UObject::GetTotalCounts()
{
    return UObject::TotalAllocationCounts;
}

