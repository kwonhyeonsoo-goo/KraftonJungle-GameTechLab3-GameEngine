#pragma once
#include "../CoreMinimal.h"
#include <functional>
#include <random>

class ObjectFactory
{
public:
    using CreateFunc = std::function<std::shared_ptr<UObject>(uint32, size_t)>;

    static auto& Registry()
    {
        static TMap<size_t, CreateFunc> registry;
        return registry;
    }

    template<typename TObject>
    static void RegisterType(size_t typeId)
    {
        static_assert(std::is_base_of_v<UObject, TObject>, "TObject must derive from UObject");

        Registry()[typeId] = [typeId](uint32 uuid, size_t tid) -> std::shared_ptr<UObject> {
            return std::shared_ptr<TObject>(new TObject(uuid, typeId));
        };
    }

    static std::shared_ptr<UObject> CreateObject(uint32 uuid, size_t typeId)
    {
        auto& reg = Registry();
        auto it = reg.find(typeId);
        if (it != reg.end())
        {
            return it->second(uuid, typeId);
        }
        return nullptr;
    }

    static uint32 GenerateUUID()
    {
        static uint32 lastUUID = 0;
        return ++lastUUID;
    }
};