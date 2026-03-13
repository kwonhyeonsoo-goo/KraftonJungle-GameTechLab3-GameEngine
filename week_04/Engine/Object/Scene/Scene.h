#pragma once
#include "Object/Object.h"

class AActor;

class ENGINE_API UScene : public UObject
{
public:
	static UClass* StaticClass();

	UScene(UClass* InClass, const FString& InName, UObject* InOuter = nullptr)
		: UObject(InClass, InName, InOuter)
	{
		
	}

private:
	TArray<AActor*> Actors;
};

