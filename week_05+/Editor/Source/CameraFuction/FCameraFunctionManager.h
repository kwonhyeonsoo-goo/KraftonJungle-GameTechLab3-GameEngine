#pragma once
#include "ICameraFunction.h"
#include "CoreMinimal.h"

class FCameraFunctionManager
{
	TArray<ICameraFunction*> ActiveFunctions;
 
public:
	void AddFunction(ICameraFunction* NewFunction) { ActiveFunctions.push_back(NewFunction); }
	void Tick(float DeltaTime);
};

