#pragma once
#include "CoreMinimal.h"
#include "EngineAPI.h"
#include "Core/ShowFlags.h"

class AActor;
class FFrustum;
class FRenderCommandQueue;
class UPrimitiveComponent;
class ENGINE_API  FSceneRenderCollector
{
public:
	void CollectRenderCommands(const TArray<AActor*>& Actors, const FFrustum& Frustum, FRenderCommandQueue& OutQueue);
	void FrustrumCull(const TArray<AActor*>& Actors, const FFrustum& Frustum, TArray<UPrimitiveComponent*>& OutVisible);

	FShowFlags& GetShowFlags() { return ShowFlags; }
	const FShowFlags& GetShowFlags() const { return ShowFlags; }
private:
	FShowFlags ShowFlags;
};