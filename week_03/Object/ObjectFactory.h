#pragma once

#include <iostream>
#include <functional>
#include "Object/Object.h"

#define REGISTER_FACTORY(TypeName)																		\
namespace {																								\
	 struct TypeName##_RegisterFactory {																\
		TypeName##_RegisterFactory() {																	\
				FObjectFactory::Get().Register(															\
					#TypeName,																			\
					[]()->weak_ptr<UObject> {return UObjectManager::Get().CreateObject<TypeName>();}	\
				);																						\
		}																								\
	};																									\
TypeName##_RegisterFactory G##TypeName##_RegisterFactory;} 																												

// Different from UFactory class
class FObjectFactory {
public:
	static FObjectFactory& Get() {
		static FObjectFactory FactorySingleton;
		return FactorySingleton;
	}

	void Register(const char* TypeName, std::function<weak_ptr<UObject>()> Spawner) {
		Registry[TypeName] = Spawner;
	}

	weak_ptr<UObject> Create(const std::string& TypeName) {
		auto Spawner = Registry.find(TypeName);	// Do NOT use array accessor [] here. it will insert a new key if not found.
		return (Spawner != Registry.end()) ? Spawner->second() : weak_ptr<UObject>();
	}

private:
	TMap<std::string, std::function<weak_ptr<UObject>()>> Registry;
};
