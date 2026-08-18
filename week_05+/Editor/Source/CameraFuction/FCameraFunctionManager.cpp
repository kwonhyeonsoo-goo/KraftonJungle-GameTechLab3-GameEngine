#include "FCameraFunctionManager.h"
#include <assert.h>


void FCameraFunctionManager::Tick(float DeltaTime)
{
	for(int i = 0; i < ActiveFunctions.size(); i++)
	{
		ICameraFunction* Function = ActiveFunctions[i]; 
		if (Function)
			Function->Tick(DeltaTime);

		
		if (!Function || Function->IsFinished())
		{
			ActiveFunctions[i--] = ActiveFunctions.back();
			ActiveFunctions.pop_back();
		}
		
	}
}