#pragma once
#include "../CoreMinimal.h"
#include "ObjectFactory.h"
#include <memory>
#include <algorithm>


class ENGINE_API ObjectManager
{
public:
    ObjectManager();
    ~ObjectManager(); // 소멸자 추가 권장

    // 템플릿: 타입별 고유 ID 생성 (정의가 헤더에 있어야 함)
    template<typename T>
    static size_t GetTypeID()
    {
        static const char tag = 0;
        return reinterpret_cast<size_t>(&tag);
    }

    template<typename TObject>
    std::shared_ptr<TObject> SpawnObject()
    {
        // 1. 타입 정보와 새로운 UUID 준비
        size_t typeId = GetTypeID<TObject>();
        uint32 newUUID = ObjectFactory::GenerateUUID();

        // 2. Factory를 통해 생성 (UUID와 TypeID가 생성자로 주입됨)
        auto basePtr = ObjectFactory::CreateObject(newUUID , typeId);

        // 3. 타입 캐스팅 및 매니저 등록
        auto typedPtr = std::static_pointer_cast<TObject>(basePtr);

        if (typedPtr)
        {
            AddToManager(std::static_pointer_cast<UObject>(typedPtr), sizeof(TObject));
        }
        return typedPtr;
    }

    // 일반 함수로 분리
    void ReleaseObject(std::shared_ptr<UObject> obj);

private:
    // 템플릿 내부에서 호출할 헬퍼 함수 (CPP에서 구현)
    void AddToManager(std::shared_ptr<UObject> obj);

    TArray<std::shared_ptr<UObject>> objectArray;

};