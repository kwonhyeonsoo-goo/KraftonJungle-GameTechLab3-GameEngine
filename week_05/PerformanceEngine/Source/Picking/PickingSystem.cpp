#include "Picking/PickingSystem.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "Camera/Camera.h"
#include "Scene/Scene.h"
#include "StaticMesh/StaticMesh.h"
#include "Visibility/VisibilitySystem.h"
#include "Scene/SceneGraph.h"
#include "Gizmo/Gizmo.h"

namespace
{
	uint64 QueryCycles64()
	{
		LARGE_INTEGER Counter = {};
		QueryPerformanceCounter(&Counter);
		return static_cast<uint64>(Counter.QuadPart);
	}

	double GetSecondsPerCycle()
	{
		static const double SecondsPerCycle = []()
		{
			LARGE_INTEGER Frequency = {};
			QueryPerformanceFrequency(&Frequency);
			return 1.0 / static_cast<double>(Frequency.QuadPart);
		}();
		return SecondsPerCycle;
	}

	double CyclesToMilliseconds(uint64 InStartCycles, uint64 InEndCycles)
	{
		return static_cast<double>(InEndCycles - InStartCycles) * GetSecondsPerCycle() * 1000.0;
	}

	bool IntersectRayAabb(const FRay& InRay, const FVector& InBoundsMin, const FVector& InBoundsMax)
	{
		float TMin = 0.0f;
		float TMax = std::numeric_limits<float>::max();

		for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
		{
			const float Origin = InRay.Origin[AxisIndex];
			const float Direction = InRay.Direction[AxisIndex];
			const float BoundsMin = InBoundsMin[AxisIndex];
			const float BoundsMax = InBoundsMax[AxisIndex];

			if (std::fabs(Direction) < 1.e-8f)
			{
				if (Origin < BoundsMin || Origin > BoundsMax) return false;
				continue;
			}

			const float InverseDirection = 1.0f / Direction;
			float T0 = (BoundsMin - Origin) * InverseDirection;
			float T1 = (BoundsMax - Origin) * InverseDirection;
			if (T0 > T1) std::swap(T0, T1);

			TMin = std::max(TMin, T0);
			TMax = std::min(TMax, T1);
			if (TMin > TMax) return false;
		}

		return true;
	}

	bool IntersectRayTriangle(
		const FRay& InRay,
		const FVector& InA,
		const FVector& InB,
		const FVector& InC,
		float& OutDistance,
		FVector& OutWorldPosition)
	{
		const FVector EdgeAB = InB - InA;
		const FVector EdgeAC = InC - InA;
		const FVector PVector = FVector::CrossProduct(InRay.Direction, EdgeAC);
		const float Determinant = FVector::DotProduct(EdgeAB, PVector);
		if (std::fabs(Determinant) < 1.e-8f) return false;

		const float InverseDeterminant = 1.0f / Determinant;
		const FVector TVector = InRay.Origin - InA;
		const float U = FVector::DotProduct(TVector, PVector) * InverseDeterminant;
		if (U < 0.0f || U > 1.0f) return false;

		const FVector QVector = FVector::CrossProduct(TVector, EdgeAB);
		const float V = FVector::DotProduct(InRay.Direction, QVector) * InverseDeterminant;
		if (V < 0.0f || U + V > 1.0f) return false;

		const float T = FVector::DotProduct(EdgeAC, QVector) * InverseDeterminant;
		if (T <= 0.0f) return false;

		OutDistance = T;
		OutWorldPosition = InRay.Origin + InRay.Direction * T;
		return true;
	}

	bool IntersectRenderItem(const FRay& InRay, const FScenePrimitiveRuntimeData& InPrimitiveRuntimeData, FPickHit& InOutBestHit)
	{
		FStaticMesh* StaticMesh = InPrimitiveRuntimeData.StaticMesh;
		if (StaticMesh == nullptr || !StaticMesh->IsValid()) return false;

		if (!IntersectRayAabb(InRay, InPrimitiveRuntimeData.WorldBounds.Min, InPrimitiveRuntimeData.WorldBounds.Max)) return false;

		const TArray<FStaticMeshVertex>& Vertices = StaticMesh->GetVertices();
		const TArray<uint32>& Indices = StaticMesh->GetIndices();
		bool bHit = false;

		//버텍스를 전부 미리 transformposition 해놓기
		TArray<FVector> TransformedVertices;
		TransformedVertices.resize(Vertices.size());
		const auto XM = InPrimitiveRuntimeData.WorldMatrix.ToXMMatrix();

		for (size_t i = 0; i < Vertices.size(); ++i)
		{
			auto V = DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&Vertices[i].Position));
			auto T = DirectX::XMVector3TransformCoord(V, XM);
			TransformedVertices[i] = FVector(T);
		}

		for (size_t IndexOffset = 0; IndexOffset + 2 < Indices.size(); IndexOffset += 3)
		{
			const uint32 IndexA = Indices[IndexOffset + 0];
			const uint32 IndexB = Indices[IndexOffset + 1];
			const uint32 IndexC = Indices[IndexOffset + 2];
			if (IndexA >= Vertices.size() || IndexB >= Vertices.size() || IndexC >= Vertices.size()) continue;

			const FVector A = TransformedVertices[IndexA];
			const FVector B = TransformedVertices[IndexB];
			const FVector C = TransformedVertices[IndexC];

			float HitDistance = 0.0f;
			FVector HitPosition = FVector::ZeroVector;
			if (!IntersectRayTriangle(InRay, A, B, C, HitDistance, HitPosition)) continue;

			const float DistanceSquared = FVector::DistSquared(InRay.Origin, HitPosition);
			if (DistanceSquared >= InOutBestHit.DistanceSquared) continue;

			InOutBestHit.DistanceSquared = DistanceSquared;
			InOutBestHit.PrimitiveId = InPrimitiveRuntimeData.PrimitiveId;
			InOutBestHit.WorldPosition = HitPosition;
			bHit = true;
		}

		return bHit;
	}
}

FRay FPickingSystem::BuildPickRay(const FCamera& InCamera, int32 InMouseX, int32 InMouseY, int32 InViewportWidth, int32 InViewportHeight)
{
	FRay Result = {};
	Result.Origin = InCamera.GetLocation();
	Result.Direction = InCamera.GetRotation().GetForwardVector();

	if (InViewportWidth <= 0 || InViewportHeight <= 0) return Result;

	const float PixelX = (static_cast<float>(InMouseX) + 0.5f) / static_cast<float>(InViewportWidth);
	const float PixelY = (static_cast<float>(InMouseY) + 0.5f) / static_cast<float>(InViewportHeight);
	const float NdcX = PixelX * 2.0f - 1.0f;
	const float NdcY = 1.0f - PixelY * 2.0f;

	const FMatrix ViewProjection = InCamera.GetViewMatrix() * InCamera.GetProjectionMatrix();
	const FMatrix InverseViewProjection = ViewProjection.GetInverse();
	auto TransformProjected = [](const FMatrix& Mat, const FVector& V) -> FVector
	{
		float X = V.X * Mat.M[0][0] + V.Y * Mat.M[1][0] + V.Z * Mat.M[2][0] + Mat.M[3][0];
		float Y = V.X * Mat.M[0][1] + V.Y * Mat.M[1][1] + V.Z * Mat.M[2][1] + Mat.M[3][1];
		float Z = V.X * Mat.M[0][2] + V.Y * Mat.M[1][2] + V.Z * Mat.M[2][2] + Mat.M[3][2];
		float W = V.X * Mat.M[0][3] + V.Y * Mat.M[1][3] + V.Z * Mat.M[2][3] + Mat.M[3][3];
		if (std::abs(W) > 1e-8f)
		{
			return FVector(X / W, Y / W, Z / W); 
		}
		return FVector(X, Y, Z);
	};
	// use TransformProjected
	const FVector WorldNear = TransformProjected(InverseViewProjection, FVector(NdcX, NdcY, 0.0f));
	const FVector WorldFar = TransformProjected(InverseViewProjection, FVector(NdcX, NdcY, 1.0f));

	const FVector Direction = (WorldFar - WorldNear).GetSafeNormal();
	if (!Direction.IsNearlyZero())
	{
		Result.Direction = Direction;
	}

	return Result;
}

void FPickingSystem::Reset()
{
}

void FPickingSystem::UpdatePick(
	const FScene& InScene,
	const FCamera& InCamera,
	const FVisibilityResults& InVisibilityResults,
	POINT InMousePositionClient,
	int32 InViewportWidth,
	int32 InViewportHeight,
	const FSceneGraph& InSceneGraph,
	FGizmo* InGizmo,
	const FMatrix* InSelectedMatrix,
	FPickState& InOutPickState) const
{
	if (InViewportWidth <= 0 || InViewportHeight <= 0) return;
	if (InMousePositionClient.x < 0 || InMousePositionClient.y < 0 || InMousePositionClient.x >= InViewportWidth || InMousePositionClient.y >= InViewportHeight) return;

	const FRay PickRay = BuildPickRay(InCamera, InMousePositionClient.x, InMousePositionClient.y, InViewportWidth, InViewportHeight);
	const uint64 PickStartCycles = QueryCycles64();

	//기즈모 피킹 판정 
	InOutPickState.bHitGizmo = false;
	InOutPickState.HitGizmoAxis = EGizmoAxis::None;

	if (InGizmo && InSelectedMatrix)
	{
		EGizmoAxis HitAxis = InGizmo->HitTestAxis(InSelectedMatrix, const_cast<FCamera*>(&InCamera), PickRay);
		if (HitAxis != EGizmoAxis::None)
		{
			InOutPickState.bHitGizmo = true;
			InOutPickState.HitGizmoAxis = HitAxis;
			InOutPickState.LastPickTimeMs = CyclesToMilliseconds(PickStartCycles, QueryCycles64());
			return; // 기즈모를 클릭했으므로 뒤에 있는 씬 오브젝트 피킹은 건너뜁니다.
		}
	}

	//씬 오브젝트 피킹
	const TArray<FScenePrimitiveRuntimeData>& PrimitiveRuntimeData = InScene.GetPrimitiveRuntimeData();
	FPickHit BestHit;
	BestHit.DistanceSquared = std::numeric_limits<float>::max();

	//AABB true만 전달
	TArray<int32> OutIndices;
	InSceneGraph.Pick(PickRay, InVisibilityResults, OutIndices);

	//for (uint32 PrimitiveIndex : InVisibilityResults.VisiblePrimitiveIndices)
	for (uint32 PrimitiveIndex : OutIndices)
	{
		if (PrimitiveIndex >= PrimitiveRuntimeData.size()) continue;

		if (IntersectRenderItem(PickRay, PrimitiveRuntimeData[PrimitiveIndex], BestHit))
		{
			BestHit.PrimitiveIndex = static_cast<int32>(PrimitiveIndex);
		}
	}

	const uint64 PickEndCycles = QueryCycles64();
	InOutPickState.LastPickTimeMs = CyclesToMilliseconds(PickStartCycles, PickEndCycles);
	InOutPickState.TotalPickTimeMs += InOutPickState.LastPickTimeMs;
	++InOutPickState.TotalPickCount;

	// 기즈모를 누르지 않았을 때만 씬 오브젝트 선택 결과를 갱신합니다.
	InOutPickState.bHit = BestHit.PrimitiveId >= 0;
	InOutPickState.SelectedPrimitiveId = BestHit.PrimitiveId;
	InOutPickState.SelectedPrimitiveIndex = BestHit.PrimitiveIndex;
	InOutPickState.HitWorldPosition = BestHit.WorldPosition;
}