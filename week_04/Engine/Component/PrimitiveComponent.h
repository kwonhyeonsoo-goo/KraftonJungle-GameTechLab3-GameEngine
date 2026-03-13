#pragma once
#include "SceneComponent.h"
#include "Primitive/PrimitiveBase.h"
#include <memory>

class ENGINE_API UPrimitiveComponent : public USceneComponent
{
public:
	CPrimitiveBase* GetPrimitive() const { return Primitive.get(); }

protected:
	std::unique_ptr<CPrimitiveBase> Primitive;
};
