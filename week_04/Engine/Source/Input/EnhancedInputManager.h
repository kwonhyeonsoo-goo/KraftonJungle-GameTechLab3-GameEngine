#pragma once
#include "CoreMinimal.h"
#include <functional>

class CInputManager;
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
		int* Context;
		int32 Priority;
	};

	struct FBindingEntry
	{
		void* Action;
		int TriggerEvent;
		void* Callback;
	};
	TArray<FMappingContextEntry> MappingContexts;
	TArray<FBindingEntry> Binding;
	TMap<void*, void> ActionStates;
};