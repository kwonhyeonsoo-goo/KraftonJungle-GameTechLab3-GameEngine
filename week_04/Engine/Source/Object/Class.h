#pragma once
#include "CoreMinimal.h"
class UObject;

class ENGINE_API UClass
{
public:
	using CreateFunc = UObject * (*)(UObject* InOuter, const FString& InName);

	UClass(FString InName, UClass* InSuperClass, CreateFunc InCreateFunc);
	
	const FString& GetName() const;
	UClass* GetSuperClass() const;

	bool IsChildOf(const UClass* Other) const;

	UObject* CreateInstance(UObject* InOuter, const FString& InName) const;

private:
	FString Name;
	UClass* SuperClass = nullptr;
	CreateFunc Factory = nullptr;
};

