#include "ObjectManager.h"


template<class T>
T* ObjectManager::SpawnObject() {

    size_t id = GetTypeID<T>();
    auto* IT =  ObjectFactory::CreateObject(id);

    if (IT != nullptr)
    {
        ObjectArray.push(IT);
    }

    return IT;
}
