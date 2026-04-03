#pragma once
#include "Types/Array.h"
class FStaticMesh;

class UObject
{
public:

private:
	FStaticMesh* StaticMesh = nullptr;

};

extern TArray<UObject*> GUObjectArray;
