#include "Object/Object.h"
#include "Object/Class.h"

// 조건 1: 전역 오브젝트 배열 정의
TArray<UObject*> GUObjectArray;

// ─────────────────────────────────────────────────────────────
//  생성 / 소멸
// ─────────────────────────────────────────────────────────────

UObject::UObject(UClass* InClass, FString InName, UObject* InOuter)
	: Class(InClass), Name(std::move(InName)), Outer(InOuter)
{
	// UUID, InternalIndex는 FObjectFactory::ConstructObject에서 주입
}

UObject::~UObject()
{
	// 조건 1: 소멸 시 GUObjectArray 슬롯을 nullptr로 마킹
	if (InternalIndex < static_cast<uint32>(GUObjectArray.Num()))
	{
		GUObjectArray[static_cast<int32>(InternalIndex)] = nullptr;
	}
}

// ─────────────────────────────────────────────────────────────
//  조건 2: 메모리 통계
// ─────────────────────────────────────────────────────────────

int32 UObject::GetTotalBytes()
{
	return static_cast<int32>(UObject::TotalAllocationBytes);
}

int32 UObject::GetTotalCounts()
{
	return static_cast<int32>(UObject::TotalAllocationCounts);
}

void* UObject::operator new(size_t InSize)
{
	UObject::TotalAllocationCounts += 1;
	UObject::TotalAllocationBytes += static_cast<uint32>(InSize);
	return ::operator new(InSize);
}

void UObject::operator delete(void* InAddress, std::size_t size)
{
	UObject::TotalAllocationCounts -= 1;
	UObject::TotalAllocationBytes -= static_cast<uint32>(size);
	::operator delete(InAddress);
}

// ─────────────────────────────────────────────────────────────
//  조건 4: RTTI
// ─────────────────────────────────────────────────────────────

namespace
{
	UObject* CreateUObjectInstance(UObject* InOuter, const FString& InName)
	{
		return new UObject(UObject::StaticClass(), InName, InOuter);
	}
}

UClass* UObject::StaticClass()
{
	static UClass ClassInfo("UObject", nullptr, &CreateUObjectInstance);
	return &ClassInfo;
}

UClass* UObject::GetClass() const
{
	return Class;
}

bool UObject::IsA(const UClass* InClass) const
{
	return Class && InClass && Class->IsChildOf(InClass);
}

// ─────────────────────────────────────────────────────────────
//  오브젝트 정보
// ─────────────────────────────────────────────────────────────

const FString& UObject::GetName() const
{
	return Name;
}

UObject* UObject::GetOuter() const
{
	return Outer;
}

FString UObject::GetPathName() const
{
	if (Outer == nullptr)
	{
		return Name;
	}

	return Outer->GetPathName() + "." + Name;
}

// ─────────────────────────────────────────────────────────────
//  플래그
// ─────────────────────────────────────────────────────────────

bool UObject::HasAnyFlags(EObjectFlags InFlags) const
{
	return static_cast<uint32>(Flags & InFlags) != 0;
}

bool UObject::HasAllFlags(EObjectFlags InFlags) const
{
	return (static_cast<uint32>(Flags & InFlags) == static_cast<uint32>(InFlags));
}

void UObject::AddFlags(EObjectFlags InFlags)
{
	Flags |= InFlags;
}

void UObject::ClearFlags(EObjectFlags InFlags)
{
	Flags = static_cast<EObjectFlags>(static_cast<uint32>(Flags) & ~static_cast<uint32>(InFlags));
}

void UObject::MarkPendingKill()
{
	AddFlags(EObjectFlags::PendingKill);
}

bool UObject::IsPendingKill() const
{
	return HasAnyFlags(EObjectFlags::PendingKill);
}