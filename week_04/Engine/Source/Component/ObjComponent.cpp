#include "ObjComponent.h"
#include "Primitive/PrimitiveObj.h"
#include "Object/Class.h"

IMPLEMENT_RTTI(UObjComponent, UPrimitiveComponent)

void UObjComponent::Initialize()
{ 
}

void UObjComponent::LoadPrimitive(const FString& FilePath)
{
	Primitive = std::make_unique<CPrimitiveObj>(FilePath);

}
