#include "ObjectManager.h"


UObject* ObjectManager::SpawnObject(ObjectType className)
{

	auto IT = ClassDefinitions.find(className);
	
	if(IT !- classDefinitions.end())
	{
		UObject* NewObject = ObjectFactory::CreateObject(IT->second);

		ObjectArray.push(NewObject);

		return NewObject;

	}

	return nullptr;

};