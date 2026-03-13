#pragma once
#include "../CoreMinimal.h"

using CreateFunc = UObject * (*)();


template<class T>
class ENGINE_API ObjectFactory
{
public:
	static TMap<size_t, CreateFunc> Registry;

	static T* CreateObject(size_t InId)
	{
		if (Registry.find(InId) != Registry.end())
		{
			return Registry[InId]();
		}

		return nullptr;
	}


	template<typename T>
	static void RegisterType(size_t InId)
	{
		Registry[InId] = []() -> UObject* { return new T(); };
	}
};