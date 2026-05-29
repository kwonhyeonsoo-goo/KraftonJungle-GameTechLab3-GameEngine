#pragma once

#include <cstddef>

#include "Object/Object.h"

// Serial-number based weak UObject pointer. It never owns the object and does
// not keep it alive for GC. Get() rejects PendingKill/Garbage objects; cleanup
// code may use GetEvenIfPendingKill() when it explicitly needs teardown access.
template<typename T>
class TWeakObjectPtr
{
public:
	TWeakObjectPtr() = default;

	TWeakObjectPtr(T* InObject)
	{
		Reset(InObject);
	}

	TWeakObjectPtr& operator=(T* InObject)
	{
		Reset(InObject);
		return *this;
	}

	void Reset(T* InObject = nullptr)
	{
		UObject* LiveObject = GetAliveObjectFromAddress(InObject);
		if (!LiveObject)
		{
			TypedObject = nullptr;
			Object = nullptr;
			SerialNumber = 0;
			return;
		}

		TypedObject = InObject;
		Object = LiveObject;
		SerialNumber = LiveObject->GetSerialNumber();
	}

	T* Get() const
	{
		UObject* LiveObject = GetAliveObjectFromAddress(Object);
		if (!LiveObject || LiveObject->GetSerialNumber() != SerialNumber)
		{
			return nullptr;
		}

		if (LiveObject->HasAnyFlags(RF_PendingKill | RF_Garbage))
		{
			return nullptr;
		}

		return TypedObject;
	}

	T* GetEvenIfPendingKill() const
	{
		UObject* LiveObject = GetAliveObjectFromAddress(Object);
		if (!LiveObject || LiveObject->GetSerialNumber() != SerialNumber)
		{
			return nullptr;
		}

		return TypedObject;
	}

	T* GetAlive() const { return GetEvenIfPendingKill(); }
	bool IsValid() const { return Get() != nullptr; }
	bool bValid() const { return IsValid(); }

	explicit operator bool() const { return IsValid(); }
	operator T*() const { return Get(); }
	T* operator->() const { return Get(); }

	bool operator==(std::nullptr_t) const { return Get() == nullptr; }
	bool operator!=(std::nullptr_t) const { return Get() != nullptr; }
	bool operator==(const T* Other) const { return Get() == Other; }
	bool operator!=(const T* Other) const { return Get() != Other; }

private:
	T* TypedObject = nullptr;
	UObject* Object = nullptr;
	uint32 SerialNumber = 0;
};
