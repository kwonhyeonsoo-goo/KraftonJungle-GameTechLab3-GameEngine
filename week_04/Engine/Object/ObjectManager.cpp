#include "ObjectManager.h"

ObjectManager::ObjectManager()
{
}

ObjectManager::~ObjectManager()
{
    // 필요 시 모든 객체 강제 해제 로직
    objectArray.clear();
}

void ObjectManager::AddToManager(std::shared_ptr<UObject> obj)
{
    if (obj)
    {
        objectArray.push_back(obj);

    }
}

void ObjectManager::ReleaseObject(std::shared_ptr<UObject> obj)
{
    if (!obj) return;

    auto it = std::remove(objectArray.begin(), objectArray.end(), obj);
    if (it != objectArray.end())
    {

        objectArray.erase(it, objectArray.end());
    }
}
