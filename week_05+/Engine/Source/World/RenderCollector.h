#pragma once
#include "CoreMinimal.h"
#include "EngineAPI.h"
#include "Core/ShowFlags.h"

class AActor;
class FCamera;
class FFrustum;
struct FRenderCommandQueue;
class UPrimitiveComponent;
class ENGINE_API FLevelRenderCollector
{
public:
	void CollectRenderCommands(const TArray<AActor*>& Actors, const FFrustum& Frustum,
		const FShowFlags& ShowFlags, const FCamera* Camera, FRenderCommandQueue& OutQueue);
	void FrustrumCull(const TArray<AActor*>& Actors, const FFrustum& Frustum,
		const FShowFlags& ShowFlags, TArray<UPrimitiveComponent*>& OutVisible);
};
