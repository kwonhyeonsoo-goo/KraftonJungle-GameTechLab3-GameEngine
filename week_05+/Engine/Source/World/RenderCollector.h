#pragma once
#include "CoreMinimal.h"
#include "EngineAPI.h"
#include "Core/ShowFlags.h"
#include "Renderer/RenderCommand.h"
#include "Math/Frustum.h"
class AActor;
class FCamera;
class FFrustum;
struct FRenderCommandQueue;
class UPrimitiveComponent;
class ENGINE_API FLevelRenderCollector
{
public:
	FLevelRenderCollector() = default;

	void MarkDirty() { bCommandsDirty = true; }
	void CollectRenderCommands(const TArray<AActor*>& Actors, const FFrustum& Frustum,
		const FShowFlags& ShowFlags, const FCamera* Camera, FRenderCommandQueue& OutQueue);
	void FrustrumCull(const TArray<AActor*>& Actors, const FFrustum& Frustum,
		const FShowFlags& ShowFlags, TArray<UPrimitiveComponent*>& OutVisible);

private:


	// ─── 캐싱 관련 멤버 ───
	FRenderCommandQueue CachedQueue;
	FFrustum PrevFrustum;
	uint64 PrevShowFlagsBits = 0; // ShowFlags 변경 감지용
	size_t PrevActorCount = 0;
	bool bCommandsDirty = true;
	bool bHasAnimatedComponents = false; // SubUV 등 매 프레임 갱신 필요
};
