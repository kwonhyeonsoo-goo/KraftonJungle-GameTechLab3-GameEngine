#include "InputMappingContext.h"


FActionKeyMapping& FInputMappingContext::AddMapping(FInputAction* Action, int32 Key)
{
	FActionKeyMapping& Mapping = Mappings.emplace_back();
	Mapping.Action = Action;
	Mapping.Key = Key;
	return Mapping;
}
