#pragma once
#include "CoreMinimal.h"
#include "ObjectTypes.h"

class UClass;

// ────────────────────────────────────────────────────────────────────────────
//  전역 오브젝트 배열 (조건 1)
//  - 엔진에서 생성된 모든 UObject 포인터를 관리
//  - InternalIndex 는 이 배열에서의 인덱스와 동일
//  - 소멸 시 해당 슬롯을 nullptr 로 마킹 (Slot 재사용은 GC 담당)
// ────────────────────────────────────────────────────────────────────────────
extern ENGINE_API TArray<UObject*> GUObjectArray;


// ────────────────────────────────────────────────────────────────────────────
//  UObject  –  엔진 내 모든 클래스의 최상위 부모 (조건 1, 2, 4)
// ────────────────────────────────────────────────────────────────────────────
class ENGINE_API UObject
{
public:
	// ── 생성 / 소멸 ──────────────────────────────────────────────────────
	/**
	 * FObjectFactory::ConstructObject 가 호출하는 생성자.
	 * 직접 new 하지 말고 반드시 FObjectFactory 를 통해 생성할 것.
	 */
	UObject(UClass* InClass, FString InName, UObject* InOuter = nullptr);

	/** GUObjectArray 슬롯을 nullptr 로 마킹 */
	virtual ~UObject();

	// ── 메모리 추적 오버로딩 (조건 2) ────────────────────────────────────
	void* operator new(size_t InSize);
	void  operator delete(void* InPtr, size_t InSize);

	/** 현재까지 할당된 총 바이트 수 */
	inline static uint32 TotalAllocationBytes = 0;
	/** 현재 살아있는 UObject 인스턴스 수 */
	inline static uint32 TotalAllocationCount = 0;

	static uint32 GetTotalBytes() { return TotalAllocationBytes; }
	static uint32 GetTotalCounts() { return TotalAllocationCount; }

	// ── GUObjectArray 식별자 (조건 1) ────────────────────────────────────
	uint32 UUID = 0; ///< 엔진 전체에서 고유한 ID (1-based, 단조 증가)
	uint32 InternalIndex = 0; ///< GUObjectArray 내의 인덱스

	// ── RTTI (조건 4) ─────────────────────────────────────────────────────
	/** UObject 자신의 UClass 반환 */
	static UClass* StaticClass();

	/** 이 인스턴스의 런타임 클래스 반환 */
	UClass* GetClass() const;

	/**
	 * 이 객체가 InClass 이거나 그 자식 클래스인지 확인.
	 * 사용 예) if (pObj->IsA(USphere::StaticClass()))
	 */
	bool IsA(const UClass* InClass) const;

	/**
	 * 템플릿 버전: if (pObj->IsA<USphere>())
	 */
	template<typename T>
	bool IsA() const
	{
		static_assert(std::is_base_of_v<UObject, T>, "T must derive from UObject");
		return IsA(T::StaticClass());
	}

	/**
	 * 안전한 다운캐스트 (실패 시 nullptr).
	 * 사용 예) USphere* s = Cast<USphere>(pObj);
	 */
	template<typename T>
	T* Cast()
	{
		return IsA<T>() ? static_cast<T*>(this) : nullptr;
	}

	template<typename T>
	const T* Cast() const
	{
		return IsA<T>() ? static_cast<const T*>(this) : nullptr;
	}

	// ── 오브젝트 정보 ────────────────────────────────────────────────────
	const FString& GetName()  const;
	UObject* GetOuter() const;
	FString        GetPathName() const;

	/**
	 * Outer 체인을 거슬러 올라가며 T 타입의 Outer 를 탐색.
	 */
	template<typename T>
	T* GetTypedOuter() const
	{
		static_assert(std::is_base_of_v<UObject, T>, "T must derive from UObject");
		for (UObject* Current = Outer; Current; Current = Current->GetOuter())
		{
			if (Current->IsA(T::StaticClass()))
				return static_cast<T*>(Current);
		}
		return nullptr;
	}

	// ── 플래그 ───────────────────────────────────────────────────────────
	bool HasAnyFlags(EObjectFlags InFlags) const;
	bool HasAllFlags(EObjectFlags InFlags) const;
	void AddFlags(EObjectFlags InFlags);
	void ClearFlags(EObjectFlags InFlags);

	void MarkPendingKill();
	bool IsPendingKill() const;

private:
	UClass* Class = nullptr;
	FString      Name;
	UObject* Outer = nullptr;
	EObjectFlags Flags = EObjectFlags::None;
};