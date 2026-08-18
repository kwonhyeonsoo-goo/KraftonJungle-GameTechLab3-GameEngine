#include "ClassRegistry.h"

void ClassRegistry::Register(const FString& typeName, UClass* uclass)
{
	if (typeName.empty() || uclass == nullptr) return;
	Classes[typeName] = uclass;
}

UClass* ClassRegistry::Find(const FString& typeName) const
{
	auto it = Classes.find(typeName);
	if (it == Classes.end()) return nullptr;
	return it->second;
}

bool ClassRegistry::Contains(const FString& typeName) const
{
	return Classes.find(typeName) != Classes.end();
}

TArray<FString> ClassRegistry::GetAllTypeNames() const
{
	TArray<FString> result;
	for (const auto& pair : Classes)
	{
		result.push_back(pair.first);
	}
	return result;
}




