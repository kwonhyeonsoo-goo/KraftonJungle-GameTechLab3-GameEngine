#include "ObjectManager.h"
#include "Object/Object.h"
#include "Object/Class.h"

ObjectManager::ObjectManager()
{
}

ObjectManager::~ObjectManager()
{
	// GUObjectArray에 남은 오브젝트 전부 해제
	for (UObject* Obj : GUObjectArray)
	{
		delete Obj;
	}
	GUObjectArray.Empty();
}

UObject* ObjectManager::SpawnObject(
	UClass* InClass,
	UObject* InOuter,
	const FString& InName)
{
	return FObjectFactory::ConstructObject(InClass, InOuter, InName);
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
	// nullptr 슬롯을 제거하고 살아있는 오브젝트의 InternalIndex 재조정
	int32 WriteIdx = 0;
	for (int32 ReadIdx = 0; ReadIdx < GUObjectArray.Num(); ++ReadIdx)
	{
		UObject* Obj = GUObjectArray[ReadIdx];
		if (Obj != nullptr)
		{
			Obj->InternalIndex = static_cast<uint32>(WriteIdx);
			GUObjectArray[WriteIdx] = Obj;
			++WriteIdx;
		}
	}
	GUObjectArray.SetNum(WriteIdx);
}