#include "ObjectManager.h"
#include "Object/Object.h"
#include "Object/Class.h"
#include "Debug/EngineLog.h"
#include <algorithm>
#include "Object/ObjectFactory.h"

constexpr int32 GUObjectArrayReserveSize = 32768;

ObjectManager::ObjectManager()
{
	GUObjectArray.reserve(GUObjectArrayReserveSize);
}

ObjectManager::~ObjectManager()
{
	// GUObjectArray에 남은 오브젝트 전부 해제
	for (UObject* Obj : GUObjectArray)
	{
		if (Obj)
		{
			delete Obj;
		}
	}
	GUObjectArray.clear();
}

UObject* ObjectManager::SpawnObject(
	UClass* InClass,
	UObject* InOuter,
	const FString& InName)
{
	static uint32 GlobalSerialNumberCounter = 1;
	UObject* NewObj = FObjectFactory::ConstructObject(InClass, InOuter, InName);
	int32 NewIndex=0;
	// 빈자리가 있으면 재사용 (O(1))
	if (!FreeIndices.empty())
	{
		NewIndex = FreeIndices.back();
		FreeIndices.pop_back();
		GUObjectArray[NewIndex] = NewObj;
	}
	// 빈자리가 없으면 배열끝에 추가 reserve
	else
	{
		NewIndex = static_cast<int32>(GUObjectArray.size());
		GUObjectArray.push_back(NewObj);
	}
	NewObj->InternalIndex = NewIndex;
	NewObj->SerialNumber = GlobalSerialNumberCounter++;
	return NewObj;
}

void ObjectManager::ReleaseObject(UObject* obj)
{
	if (!obj) return;

	// PendingKill 마킹 후 즉시 삭제
	// ~UObject()에서 GUObjectArray[InternalIndex] = nullptr 처리
	obj->MarkPendingKill();
	delete obj;
}

void ObjectManager::FlushKilledObjects()
{
	int32 PrevCount = static_cast<int32>(GUObjectArray.size());
	int32 KilledCount = 0;

	// Phase 1: PendingKill 오브젝트를 실제 delete (GC)
	for (int32 Idx = 0; Idx < GUObjectArray.size(); ++Idx)
	{
		UObject* Obj = GUObjectArray[Idx];
		if (!Obj)
			continue;
		// RootSet이나 Standalone 에셋은 보호
		if (Obj->HasAnyFlags(EObjectFlags::RootSet | EObjectFlags::Standalone))
		{
			continue;
		}
		if (Obj->IsPendingKill())
		{
			delete Obj;// 소멸자에서 GUObjectArray[Idx] = nullptr 처리됨
			// 빈자리를 Free List에 등록
			FreeIndices.push_back(Idx);
			++KilledCount;
		}
	}

	

	if (KilledCount > 0)
	{
		UE_LOG("[GC] Collected %d objects. Free Slots: %zu", KilledCount, FreeIndices.size());
	}
}