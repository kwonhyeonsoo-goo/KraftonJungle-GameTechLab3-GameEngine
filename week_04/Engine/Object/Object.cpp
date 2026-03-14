#include "Object/Object.h"

int32 UObject::GetTotalBytes()
{
    return UObject::TotalAllocationBytes;
}


int32 UObject::GetTotalCounts()
{
    return UObject::TotalAllocationCounts;
}

void* UObject::operator new(size_t InSize)
{

    UObject::TotalAllocationCounts += 1;
    UObject::TotalAllocationBytes += static_cast<uint32>(InSize);
    return ::operator new(InSize);
}


//반드시 소멸자를 가상으로 선언할 것
void UObject::operator delete(void* InAddress , std::size_t size)
{
    UObject::TotalAllocationCounts -= 1;
    UObject::TotalAllocationBytes -= static_cast<uint32>(size);
    ::operator delete(InAddress);

}