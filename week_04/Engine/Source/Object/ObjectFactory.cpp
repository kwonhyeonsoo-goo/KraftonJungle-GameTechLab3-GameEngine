#include "ObjectFactory.h"
#include "Object/Object.h"
#include "Object/Class.h"

// 조건 3: ConstructObject 구현
// 처리 순서:
//   1. UClass::CreateInstance  → 파생 클래스 new 호출 (operator new에서 메모리 통계 갱신)
//   2. UUID 부여
//   3. InternalIndex = GUObjectArray 현재 크기
//   4. GUObjectArray에 push
UObject* FObjectFactory::ConstructObject(
	UClass* InClass,
	UObject* InOuter,
	const FString& InName)
{
	if (!InClass)
	{
		return nullptr;
	}

	UObject* NewObj = InClass->CreateInstance(InOuter, InName);
	if (!NewObj)
	{
		return nullptr;
	}

	NewObj->UUID = GenerateUUID();
	NewObj->InternalIndex = static_cast<uint32>(GUObjectArray.size());
	GUObjectArray.push_back(NewObj);

	return NewObj;
}

uint32 FObjectFactory::GenerateUUID()
{
	static uint32 LastUUID = 0;
	return ++LastUUID;
}