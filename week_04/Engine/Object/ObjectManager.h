#pragma once
#include "../CoreMinimal.h"
#include "ObjectFactory.h"
#include <memory>
#include <algorithm>

// 상명 참조를 피하기 위해 전방 선언 활용
class UObject;

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

    // 템플릿: 객체 생성 및 참조 보관
    template<typename TObject>
    std::shared_ptr<TObject> SpawnObject()
    {
        size_t id = GetTypeID<TObject>();
        // 실제 생성 로직은 Factory에 위임
        auto basePtr = ObjectFactory::CreateObject(id);
        auto typedPtr = std::dynamic_pointer_cast<TObject>(basePtr);

        if (typedPtr)
        {
            // 내부 리스트 등록 및 통계 업데이트는 CPP 함수 호출
            AddToManager(std::static_pointer_cast<UObject>(typedPtr), sizeof(TObject));
        }
        return typedPtr;
    }

    // 일반 함수로 분리
    void ReleaseObject(std::shared_ptr<UObject> obj);

    uint32_t GetTotalAllocationBytes() const;
    uint32_t GetTotalAllocationCount() const;

private:
    // 템플릿 내부에서 호출할 헬퍼 함수 (CPP에서 구현)
    void AddToManager(std::shared_ptr<UObject> obj, size_t size);

    TArray<std::shared_ptr<UObject>> objectArray;
    uint32_t totalAllocationBytes;
    uint32_t totalAllocationCount;
};