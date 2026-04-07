#pragma once

#include "CoreMinimal.h"
#include "LevelTypes.h"

class AActor;
class ULevel;

struct ENGINE_API FLevelContext
{
	FString ContextName;
	EWorldType LevelType = EWorldType::Game;
	ULevel* Level = nullptr;

	bool IsValid() const { return Level != nullptr; }
	void Reset()
	{
		ContextName.clear();
		LevelType = EWorldType::Game;
		Level = nullptr;
	}
};

struct ENGINE_API FEditorLevelContext : public FLevelContext
{
	TObjectPtr<AActor> SelectedActor;

	void Reset()
	{
		FLevelContext::Reset();
		SelectedActor = nullptr;
	}
};
