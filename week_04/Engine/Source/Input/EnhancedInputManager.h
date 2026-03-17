#pragma once
#include "CoreMinimal.h"
#include "InputAction.h"
#include "InputTrigger.h"
#include "InputMappingContext.h"
#include <functional>

class CInputManager;
using FInputActionCallback = std::function<void(const FInputActionValue&)>;
class CEnhancedInputManager
{
public:

	void AddMappingContext();
	void RemoveContext();
	void ClearAllMappingContext();

	void BindAction();
	void ClearBinding();
	void Processinput();
private:
	struct FMappingContextEntry
	{
		FInputMappingContext* Context;
		int32 Priority;
	};

	struct FBindingEntry
	{
		FInputAction* Action;
		ETriggerEvent TriggerEvent;
		FInputActionCallback* Callback;
	};
	TArray<FMappingContextEntry> MappingContexts;
	TArray<FBindingEntry> Binding;
	TMap<FInputAction*, ETriggerState> ActionStates;
};