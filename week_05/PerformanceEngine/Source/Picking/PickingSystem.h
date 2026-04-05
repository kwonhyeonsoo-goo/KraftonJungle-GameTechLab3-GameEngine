#pragma once

#include <Windows.h>

#include "Scene/SceneTypes.h"
#include "Types/PlatformTypes.h"

class FCamera;
class FScene;
struct FVisibilityResults;
class FSceneGraph;
class FGizmo;
class FMatrix;
enum class EGizmoAxis : uint8;

// 기즈모와 피킹 시스템이 공유할 광선 구조체
struct FRay
{
	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	FVector InvDirection = FVector::OneVector;
};

struct FPickState
{
	int32 SelectedPrimitiveId = -1;
	int32 SelectedPrimitiveIndex = -1;
	bool bHit = false;
	FVector HitWorldPosition = FVector::ZeroVector;
	double LastPickTimeMs = 0.0;
	double LastMeshPickTimeMS = 0.0f;
	double LastWorldPickTimeMs = 0.0f;
	double TotalPickTimeMs = 0.0;
	uint64 TotalPickCount = 0;

	// 기즈모 피킹 결과
	bool bHitGizmo = false;
	EGizmoAxis HitGizmoAxis = static_cast<EGizmoAxis>(0); // EGizmoAxis::None
};

class FPickingSystem
{
public:
	void Reset();

	// 외부(Core, Gizmo 등)에서 광선을 생성할 수 있도록 public static으로 노출
	static FRay BuildPickRay(const FCamera& InCamera, int32 InMouseX, int32 InMouseY, int32 InViewportWidth, int32 InViewportHeight);

	void UpdatePick(
		const FScene& InScene,
		const FCamera& InCamera,
		const FVisibilityResults& InVisibilityResults,
		POINT InMousePositionClient,
		int32 InViewportWidth,
		int32 InViewportHeight,
		const FSceneGraph& InSceneGraph,
		FGizmo* InGizmo,
		const FMatrix* InSelectedMatrix,
		FPickState& InOutPickState) const;
};