#pragma once
#include "Object.h"
#include "CoreMinimal.h"
#include "SparseData.h"

class ENGINE_API ObjectManager
{
public:
	ObjectManager();
	~ObjectManager() = default;



	//Todo
	void RegisterClass(ObjectType, SparseData*);
	
	UObject* SpawnObject(ObjectType className);


private:

	TMap<ObjectType, SparseData* > ClassDefinition;




};

