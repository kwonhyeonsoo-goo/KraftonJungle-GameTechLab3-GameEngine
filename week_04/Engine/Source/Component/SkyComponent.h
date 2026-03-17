#pragma once
#include "PrimitiveComponent.h"

class ENGINE_API USkyComponent : public UPrimitiveComponent
{
public:
	static UClass* StaticClass();

	USkyComponent();
	USkyComponent(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);
};