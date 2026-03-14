#pragma once
#include "SceneComponent.h"
#include "Primitive/PrimitiveBase.h"
#include <memory>

class ENGINE_API UPrimitiveComponent : public USceneComponent
{
public:
	static UClass* StaticClass();

	UPrimitiveComponent() : USceneComponent(StaticClass(), "") {}

	UPrimitiveComponent(UClass* InClass, const FString& InName, UObject* InOuter = nullptr)
		: USceneComponent(InClass, InName, InOuter) {}

	CPrimitiveBase* GetPrimitive() const { return Primitive.get(); }

protected:
	std::unique_ptr<CPrimitiveBase> Primitive;
};
