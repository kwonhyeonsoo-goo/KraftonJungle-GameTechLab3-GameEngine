#pragma once
#include "CoreMinimal.h"
#include "World/LevelTypes.h"

class AActor;
class UWorld;

struct ENGINE_API FWorldContext
{
	FString ContextName;
	ELevelType WorldType = ELevelType::Game;
	UWorld* World = nullptr;

	bool IsValid() const { return World != nullptr; }
	void Reset()
	{
		ContextName.clear();
		WorldType = ELevelType::Game;
		World = nullptr;
	}
};

struct ENGINE_API FEditorWorldContext : public FWorldContext
{
	TObjectPtr<AActor> SelectedActor;

	void Reset()
	{
		FWorldContext::Reset();
		SelectedActor = nullptr;
	}
};
