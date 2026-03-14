#pragma once
#include "../CoreMinimal.h"
#include <functional>
#include <random>

using CreateFunc = UObject * (*)();


class ObjectFactory
{
public:
    // 생성 함수가 UUID와 TypeID를 모두 받도록 정의
    using CreateFunc = std::function<std::shared_ptr<UObject>(size_t, uint32)>;

    static auto& Registry()
    {
        static TMap<size_t, CreateFunc> registry;
        return registry;
    }

    template<typename TObject>
    static void RegisterType(size_t typeId)
    {
        static_assert(std::is_base_of_v<BaseObject, TObject>, "TObject must derive from BaseObject");

        // 람다 등록 시 UUID, typeId를 인자로 받아 생성자에 넘겨줌
        Registry()[typeId] = [typeId]( uint32 uuid , size_t tid) -> std::shared_ptr<BaseObject> {
            // tid 인자를 직접 써도 되고, 캡처한 typeId를 써도 됩니다.
            //return std::make_shared<TObject>( uuid , typeId);
            return std::shared_ptr<TObject>(new Object(uuid, typeId));


            };
    }

    static std::shared_ptr<UObject> CreateObject( uint32 uuid , size_t typeId)
    {
        auto& reg = Registry();
        auto it = reg.find(typeId);
        if (it != reg.end())
        {
            // 등록된 람다에 uuid와 typeId를 전달
            return it->second( uuid , typeId);
        }
        return nullptr;
    }

    static uint32 GenerateUUID()
    {
        static uint32 lastUUID = 0;
        return ++lastUUID;
    }
};