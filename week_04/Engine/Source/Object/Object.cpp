#include "Object/Object.h"

int32 UObject::GetTotalBytes()
{
	return UObject::TotalAllocationBytes;
}


int32 UObject::GetTotalCounts()
{
	return UObject::TotalAllocationCounts;
}

void* UObject::operator new(size_t InSize)
{

	UObject::TotalAllocationCounts += 1;
	UObject::TotalAllocationBytes += static_cast<uint32>(InSize);
	return ::operator new(InSize);
}


//반드시 소멸자를 가상으로 선언할 것
void UObject::operator delete(void* InAddress, std::size_t size)
{
	UObject::TotalAllocationCounts -= 1;
	UObject::TotalAllocationBytes -= static_cast<uint32>(size);
	::operator delete(InAddress);

}

namespace
{
	UObject* CreateUObjectInstance(UObject* InOuter, const FString& InName)
	{
		return new UObject(UObject::StaticClass(), InName, InOuter);
	}
}

UObject::UObject(UClass* InClass, FString InName, UObject* InOuter)
	: Class(InClass), Name(std::move(InName)), Outer(InOuter)
{
}

UClass* UObject::GetClass() const
{
	return Class;
}

const FString& UObject::GetName() const
{
	return Name;
}

UObject* UObject::GetOuter() const
{
	return Outer;
}

bool UObject::IsA(const UClass* InClass) const
{
	return Class && InClass && Class->IsChildOf(InClass);
}

FString UObject::GetPathName() const
{
	if (Outer == nullptr)
	{
		return Name;
	}

	return Outer->GetPathName() + "." + Name;
}

bool UObject::HasAnyFlags(EObjectFlags InFlags) const
{
	return static_cast<uint32>(Flags & InFlags) != 0;
}

bool UObject::HasAllFlags(EObjectFlags InFlags) const
{
	return (static_cast<uint32_t>(Flags & InFlags) == static_cast<uint32_t>(InFlags));
}

void UObject::AddFlags(EObjectFlags InFlags)
{
	Flags |= InFlags;
}

void UObject::ClearFlags(EObjectFlags InFlags)
{
	Flags = static_cast<EObjectFlags>(static_cast<uint32_t>(Flags) & ~static_cast<uint32_t>(InFlags));
}

void UObject::MarkPendingKill()
{
	AddFlags(EObjectFlags::PendingKill);
}

bool UObject::IsPendingKill() const
{
	return HasAnyFlags(EObjectFlags::PendingKill);
}

UClass* UObject::StaticClass()
{
	static UClass ClassInfo("UObject", nullptr, &CreateUObjectInstance);
	return &ClassInfo;
}
