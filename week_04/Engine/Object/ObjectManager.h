#pragma once
#include "../CoreMinimal.h"
#include "ObjectFactory.h";

class ENGINE_API ObjectManager
{
public:
	ObjectManager();
	~ObjectManager() = default;


    // -> Template에 따라 다른 메모리들이 할당된다.
    template<typename T>
    static size_t GetTypeID() {
        // 이 static 변수는 각 타입(T)마다 메모리에 딱 하나씩 생성됩니다.
        // 그 변수의 주소값 자체를 고유 ID로 사용하는 트릭입니다.
        static const char tag = 0;
        return reinterpret_cast<size_t>(&tag);
    }

	template <class T>
	T* SpawnObject() {

        size_t id = GetTypeID<T>();

        auto* IT = ObjectFactory::CreateObject(id);

        if (IT != nullptr)
        {
            ObjectArray.push(IT);
            RegisterAllocation(sizeof(T));

            return IT;

        }

    }

private:
    void RegisterAllocation(size_t InSize);

    TArray<UObject*> ObjectArray = {};
    uint32 TotalAllocationBytes = 0;
    uint32 TotalAllocationCount = 0;
};


