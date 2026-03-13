#include "ObjectManager.h"





ObjectManager::ObjectManager()
{

}

void ObjectManager::RegisterAllocation(size_t InSize)
{
    TotalAllocationBytes += static_cast<uint32>(InSize);
    TotalAllocationCount++;
}