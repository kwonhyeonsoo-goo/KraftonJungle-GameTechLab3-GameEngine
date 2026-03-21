#include "LineBatchComponent.h"
#include "PrimitiveLineBatch.h"

IMPLEMENT_RTTI(ULineBatchComponent, UPrimitiveComponent)

void ULineBatchComponent::Initialize()
{
	Primitive = std::make_shared<CPrimitiveLineBatch>();
	LocalBoundRadius = 1.0f;
}

void ULineBatchComponent::DrawLine(FVector InStart, FVector InEnd, FVector4 InColor)
{
	static_pointer_cast<CPrimitiveLineBatch>(Primitive)->AddLine(InStart, InEnd, InColor);
	UpdateLocalBoundRadius(InStart);
	UpdateLocalBoundRadius(InEnd);
}
