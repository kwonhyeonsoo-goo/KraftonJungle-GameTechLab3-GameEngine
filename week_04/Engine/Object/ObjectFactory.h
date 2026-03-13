#pragma once
#include "CoreMinimal.h"
#include "SparseData.h"


class ENGINE_API ObjectFactory
{
public:

	static UObject* CreateObject(const SparseData& InClassData)
	{

		UObject* NewObject = new UObject(InClassData);

		return NewObject;
	}
};