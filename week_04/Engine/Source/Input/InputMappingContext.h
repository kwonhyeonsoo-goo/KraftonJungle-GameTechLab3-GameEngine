#pragma once
#include "InputAction.h"
#include "CoreMinimal.h"
#include <memory>
class FInputModifier;
class FInputTrigger;

struct ENGINE_API FActionKeyMapping
{
	FInputAction* Action = nullptr;
	int32 Key = 0;
};
struct ENGINE_API FInputMappingContext
{
	FString ContextName;
	TArray<FActionKeyMapping> Mappings;

	FActionKeyMapping& AddMapping(FInputAction* Action, int32 Key);

};