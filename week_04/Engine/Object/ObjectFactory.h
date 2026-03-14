#pragma once
#include "../CoreMinimal.h"
#include <functional>

using CreateFunc = UObject * (*)();


class ObjectFactory
{
public:
    using CreateFunc = std::function<std::shared_ptr<UObject>()>;

    static auto& Registry()
    {
        static TMap<size_t, CreateFunc> registry;
        return registry;
    }

    template<typename TObject>
    static void RegisterType(size_t typeId)
    {
        static_assert(std::is_base_of_v<BaseObject, TObject>, "TObject must derive from UObject");
        Registry()[typeId] = []() -> std::shared_ptr<BaseObject> {
            return std::make_shared<TObject>();
            };
    }

    static std::shared_ptr<UObject> CreateObject(size_t typeId)
    {
        auto& reg = Registry();
        auto it = reg.find(typeId);
        if (it != reg.end())
        {
            return it->second();
        }
        return nullptr;
    }
};