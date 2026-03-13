#pragma once
#include "CoreMinimal.h"
#include "Object.h"
#include "SparseData.h"


class ENGINE_API ObjectFactory
{

	static UObject* CreateObject(const SparseData& InClassData)
	{

		UObject* NewObject = new UObject(InClassData);

		return NewObject;
	}
};