#include "Object.h"
#include "EngineStatics.h"

TArray<shared_ptr<UObject>> GUObjectArray;

UObject::UObject()
{
	UUID = EngineStatics::GenUUID();
	bPendingKill = false;
	//Name = FName("");	 // Set by UObjectManager::CreateObject
	InternalIndex = 0;   // Set by UObjectManager::CreateObject
	AllocationSize = 0;  // Set by UObjectManager::CreateObject
}

UObject::~UObject()
{
	EngineStatics::OnDeallocated(AllocationSize);
	//uint32 LastIndex = static_cast<uint32>(GUObjectArray.size() - 1);

	//if (InternalIndex != LastIndex)
	//{
	//	UObject* LastObject = GUObjectArray[LastIndex];

	//	GUObjectArray[InternalIndex] = LastObject;

	//	LastObject->InternalIndex = this->InternalIndex;
	//}

	//GUObjectArray.pop_back();

	//EngineStatics::OnDeallocated(sizeof(UObject));
}

const FTypeInfo UObject::s_TypeInfo = { "UObject", nullptr, sizeof(UObject) };

//#include "Engine/World.h"
//void UObjectManager::PurgeScene() {
//	for (UObject* Obj : GUObjectArray) {
//		if (Obj->IsA<UWorld>()) {
//			UWorld* World = Obj->Cast<UWorld>();
//				World->EndPlay();
//		}
//	}
//
//	CollectGarbage();
//	GUObjectArray.clear();
//}
