#pragma once
#include "Types/Array.h"
#include "EngineAPI.h"

class UObject;
extern ENGINE_API TArray<UObject*> GUObjectArray;
template<typename T = UObject>
class TWeakObjectPtr
{
public:


	int32 ObjectIndex = -1;
	uint32 ObjectSerialNumber = 0;

	TWeakObjectPtr() = default;

	TWeakObjectPtr(T* Object)
	{
		if (Object && !Object->IsPendingKill())
		{
			ObjectIndex = static_cast<int32>(Object->InternalIndex);
			ObjectSerialNumber = Object->SerialNumber;
		}
	}

	// 포인터가 아직 유효한지 검사 (핵심 로직)
	bool IsValid() const
	{
		if (ObjectIndex < 0 || ObjectIndex >= static_cast<int32>(GUObjectArray.size()))
			return false;

		UObject* Obj = GUObjectArray[ObjectIndex];
		// 슬롯에 객체가 있고, 일련번호가 내가 알던 그 번호가 맞으며, 삭제 대기 상태가 아닐 때만 true
		if (Obj && Obj->SerialNumber == ObjectSerialNumber && !Obj->IsPendingKill())
		{
			return true;
		}
		return false;
	}

	// 유효하면 포인터 반환, 아니면 nullptr 반환
	T* Get() const
	{
		return IsValid() ? static_cast<T*>(GUObjectArray[ObjectIndex]) : nullptr;
	}

	T* operator->() const { return Get(); }
	explicit operator bool() const { return IsValid(); }

	//  포인터를 명시적으로 비울 때 사용
	void Reset()
	{
		ObjectIndex = -1;
		ObjectSerialNumber = 0;
	}
	//nullptr로 초기화할 때 전방 선언 에러 방지
	TWeakObjectPtr(std::nullptr_t)
	{
		Reset();
	}
	//nullptr을 대입할 때 전방 선언 에러 방지
	TWeakObjectPtr& operator=(std::nullptr_t)
	{
		Reset();
		return *this;
	}
	// TWeakObjectPtr 끼리의 비교
	bool operator==(const TWeakObjectPtr& Other) const
	{
		return ObjectIndex == Other.ObjectIndex && ObjectSerialNumber == Other.ObjectSerialNumber;
	}
	bool operator!=(const TWeakObjectPtr& Other) const { return !(*this == Other); }

	// 실제 Raw Pointer(T*) 와의 비교 (아주 자주 쓰임!)
	bool operator==(const T* Other) const { return Get() == Other; }
	bool operator!=(const T* Other) const { return Get() != Other; }

	TWeakObjectPtr& operator=(T* Object)
	{
		if (Object && !Object->IsPendingKill())
		{
			ObjectIndex = static_cast<int32>(Object->InternalIndex);
			ObjectSerialNumber = Object->SerialNumber;
		}
		else
		{
			Reset(); // nullptr이거나 삭제 대기 중이면 깔끔하게 비움
		}
		return *this;
	}
};