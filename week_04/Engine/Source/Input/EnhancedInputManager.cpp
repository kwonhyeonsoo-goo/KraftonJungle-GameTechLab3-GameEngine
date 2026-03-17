#include "EnhancedInputManager.h"
#include "InputManager.h"
#include <algorithm>



void CEnhancedInputManager::AddMappingContext(FInputMappingContext* Context, int32 Priority)
{
	MappingContexts.push_back({ Context, Priority });
	std::sort(MappingContexts.begin(), MappingContexts.end(),
		[](const FMappingContextEntry& A, const FMappingContextEntry& B)
	{
		return A.Priority > B.Priority;
	});
}

void CEnhancedInputManager::RemoveMappingContext(FInputMappingContext* Context)
{
	MappingContexts.erase(
		std::remove_if(MappingContexts.begin(), MappingContexts.end(),
			[Context](const FMappingContextEntry& E) { return E.Context == Context; }),
		MappingContexts.end());
}

void CEnhancedInputManager::ClearAllMappingContexts()
{
	MappingContexts.clear();
}

void CEnhancedInputManager::BindAction(FInputAction* Action, ETriggerEvent TriggerEvent, FInputActionCallback Callback)
{
	Binding.push_back({ Action, TriggerEvent, std::move(Callback) });
}

void CEnhancedInputManager::ClearBindings()
{
	Binding.clear();
}


FInputActionValue CEnhancedInputManager::GetRawActionValue(CInputManager* Input, int32 Key)
{
	return FInputActionValue();
}
void CEnhancedInputManager::ProcessInput(CInputManager* RawInput, float DeltaTime)
{
}
