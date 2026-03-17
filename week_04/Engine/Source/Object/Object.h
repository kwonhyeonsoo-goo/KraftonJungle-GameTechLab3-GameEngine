#pragma once
#include "CoreMinimal.h"
#include "ObjectTypes.h"

class UClass;
class UObject;
// 조건 1: 엔진에서 생성되는 모든 UObject를 관리하는 전역 배열
// InternalIndex가 이 배열의 인덱스와 대응됨

extern ENGINE_API 	TArray<UObject*> GUObjectArray;


class ENGINE_API UObject
{
public:
	UObject(UClass* InClass, FString InName, UObject* InOuter = nullptr);
	virtual ~UObject();  // GUObjectArray 슬롯 nullptr 마킹

	// 조건 2: new/delete 오버로딩으로 메모리 통계 추적
	static int32 GetTotalBytes();
	static int32 GetTotalCounts();

	void* operator new(size_t InSize);
	void operator delete(void* InAddress, std::size_t size);

	inline static uint32 TotalAllocationBytes = 0;
	inline static uint32 TotalAllocationCounts = 0;

	// 조건 1: 모든 UObject가 갖는 고유 식별자 (FObjectFactory가 주입)
	uint32 UUID = 0; // 엔진 전체 고유 ID (1-based, 단조 증가)
	uint32 InternalIndex = 0; // GUObjectArray 내 인덱스

	// 조건 4: RTTI
	UClass* GetClass() const;
	const FString& GetName() const;
	UObject* GetOuter() const;

	bool IsA(const UClass* InClass) const;

	template <typename T>
	bool IsA() const
	{
		static_assert(std::is_base_of_v<UObject, T>, "T must derive from UObject");
		return IsA(T::StaticClass());
	}

	template <typename T>
	T* GetTypedOuter() const
	{
		static_assert(std::is_base_of_v<UObject, T>, "T must derive from UObject");

		UObject* Current = Outer;
		while (Current)
		{
			if (Current->IsA(T::StaticClass()))
			{
				return static_cast<T*>(Current);
			}
			Current = Current->GetOuter();
		}
		return nullptr;
	}

	FString GetPathName() const;

	bool HasAnyFlags(EObjectFlags InFlags) const;
	bool HasAllFlags(EObjectFlags InFlags) const;
	void AddFlags(EObjectFlags InFlags);
	void ClearFlags(EObjectFlags InFlags);

	void MarkPendingKill();
	bool IsPendingKill() const;

	static UClass* StaticClass();


private:
	UClass* Class = nullptr;
	FString      Name;
	UObject* Outer = nullptr;
	EObjectFlags Flags = EObjectFlags::None;
};


