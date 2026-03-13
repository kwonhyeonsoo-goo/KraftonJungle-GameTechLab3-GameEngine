#pragma once
#include "CoreMinimal.h"

class UScene;

class ENGINE_API AActor : public UObject
{
public:
	AActor() = default;
	~AActor() override = default;

protected:
	UScene* Scene = nullptr;

	
};

