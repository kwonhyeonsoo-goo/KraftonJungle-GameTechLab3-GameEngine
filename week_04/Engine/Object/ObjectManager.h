#pragma once
#include "CoreMinimal.h"
#include "SparseData.h"
#include "ObjectFactory.h";

class ENGINE_API ObjectManager
{
public:
	ObjectManager();
	~ObjectManager() = default;



	//Todo
	void RegisterClass(ObjectType, SparseData*);
	
	UObject* SpawnObject(ObjectType className);


private:

	TMap<ObjectType, SparseData* > ClassDefinitions;
	TArray<UObject*> ObjectArray;




};

