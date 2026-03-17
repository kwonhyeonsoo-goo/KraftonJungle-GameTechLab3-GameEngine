#pragma once
#include "PrimitiveComponent.h"
class ENGINE_API UGizmoComponent : public UPrimitiveComponent
{
public:
	static UClass* StaticClass();

	UGizmoComponent();
	UGizmoComponent(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);
};

