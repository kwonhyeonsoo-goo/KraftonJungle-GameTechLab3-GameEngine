#include "ObjectManager.h"

ObjectManager::ObjectManager()
    : totalAllocationBytes(0)
    , totalAllocationCount(0)
{
}

ObjectManager::~ObjectManager()
{
    // 필요 시 모든 객체 강제 해제 로직
    objectArray.clear();
}

void ObjectManager::AddToManager(std::shared_ptr<UObject> obj, size_t size)
{
    if (obj)
    {
        objectArray.push_back(obj);
        totalAllocationCount++;
        totalAllocationBytes += static_cast<uint32_t>(size);
    }
}

void ObjectManager::ReleaseObject(std::shared_ptr<UObject> obj)
{
    if (!obj) return;

    auto it = std::remove(objectArray.begin(), objectArray.end(), obj);
    if (it != objectArray.end())
    {
        // 삭제 전 통계 차감 (선택 사항)
         totalAllocationCount--;
         totalAllocationBytes -= sizeof(obj.get()); 

        objectArray.erase(it, objectArray.end());
    }
}

uint32_t ObjectManager::GetTotalAllocationBytes() const
{
    return totalAllocationBytes;
}

uint32_t ObjectManager::GetTotalAllocationCount() const
{
    return totalAllocationCount;
}