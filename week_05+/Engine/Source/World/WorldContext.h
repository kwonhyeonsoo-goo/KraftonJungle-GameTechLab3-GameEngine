#pragma once
#include "CoreMinimal.h"
#include "World/WorldType.h"
#include "Actor/Actor.h"

class AActor;
class UWorld;

struct ENGINE_API FWorldContext
{
	FString ContextName;
	EWorldType WorldType = EWorldType::Game;
	UWorld* World = nullptr;

	bool IsValid() const { return World != nullptr; }
	void Reset()
	{
		ContextName.clear();
		WorldType = EWorldType::Game;
		World = nullptr;
	}
};

struct ENGINE_API FEditorWorldContext : public FWorldContext
{
	TWeakObjectPtr<AActor> SelectedActor;

	void Reset()
	{
		FWorldContext::Reset();
		SelectedActor = nullptr;
	}
};
