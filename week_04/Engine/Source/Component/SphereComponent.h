#pragma once
#include "PrimitiveComponent.h"

class ENGINE_API USphereComponent : public UPrimitiveComponent
{
public:
	static UClass* StaticClass();

	USphereComponent();
	USphereComponent(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);
};
