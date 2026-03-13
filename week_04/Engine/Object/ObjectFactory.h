#pragma once
#include "../CoreMinimal.h"


template<class T>
class ENGINE_API ObjectFactory
{
public:

	static UObject* CreateObject(size_t id)
	{

		UObject* NewObject = new T(id);

		return NewObject;
	}
};