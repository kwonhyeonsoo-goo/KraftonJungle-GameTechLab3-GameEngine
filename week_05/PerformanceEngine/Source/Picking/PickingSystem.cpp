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

	bool IntersectRayAabb(const FRay& InRay, const FVector& InBoundsMin, const FVector& InBoundsMax, float& OutDistance)
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

		OutDistance = TMin; 
		return true;
	}

	inline bool IntersectRayAabbFast(const FRay& InRay, const FVector& InvDir, const FVector& InMin, const FVector& InMax, float MaxDistance, float& OutDistance)
	{
		float tx1 = (InMin.X - InRay.Origin.X) * InvDir.X;
		float tx2 = (InMax.X - InRay.Origin.X) * InvDir.X;
		float tmin = std::min(tx1, tx2);
		float tmax = std::max(tx1, tx2);

		float ty1 = (InMin.Y - InRay.Origin.Y) * InvDir.Y;
		float ty2 = (InMax.Y - InRay.Origin.Y) * InvDir.Y;
		tmin = std::max(tmin, std::min(ty1, ty2));
		tmax = std::min(tmax, std::max(ty1, ty2));

		float tz1 = (InMin.Z - InRay.Origin.Z) * InvDir.Z;
		float tz2 = (InMax.Z - InRay.Origin.Z) * InvDir.Z;
		tmin = std::max(tmin, std::min(tz1, tz2));
		tmax = std::min(tmax, std::max(tz1, tz2));

		if (tmax >= tmin && tmax > 0.0f && tmin < MaxDistance)
		{
			// 광선 시작점이 박스 안에 있으면 tmin이 음수일 수 있으므로 0으로 보정
			OutDistance = std::max(0.0f, tmin);
			return true;
		}
		return false;
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
		// std::abs(Determinant) < 1.e-8f 대신 부호를 체크합니다.
		// D3D11 기준 시계방향(CW)이 앞면이므로, Determinant가 양수일 때만 앞면입니다.
		// 광선이 뒷면을 때리면 (Determinant < 1.e-8f) 즉시 연산을 종료하여 연산량을 50% 줄입니다.
		if (Determinant < 1.e-8f) return false;

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

		// 1. 월드 바운딩 박스 1차 거르기
		float distance = std::numeric_limits<float>::max();
		if (!IntersectRayAabb(InRay, InPrimitiveRuntimeData.WorldBounds.Min, InPrimitiveRuntimeData.WorldBounds.Max, distance)) return false;

		const FBVHMesh& BVH = StaticMesh->GetBVH();
		// 🚨 최적화 1: 반드시 참조(&)로 받아 복사 방지!
		const TArray<FBVHMeshNode>& Nodes = BVH.GetNodes();
		if (Nodes.empty()) return false;

		// 2. 광선을 로컬 공간으로 변환
		const auto XM = InPrimitiveRuntimeData.WorldMatrix.ToXMMatrix();
		DirectX::XMVECTOR Determinant;
		const auto WorldToLocalXM = DirectX::XMMatrixInverse(&Determinant, XM);

		auto RayOrigin = DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&InRay.Origin));
		auto RayDir = DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&InRay.Direction));

		auto LocalOriginXM = DirectX::XMVector3TransformCoord(RayOrigin, WorldToLocalXM);
		auto LocalDirXM = DirectX::XMVector3TransformNormal(RayDir, WorldToLocalXM);
		LocalDirXM = DirectX::XMVector3Normalize(LocalDirXM);

		FRay LocalRay;
		DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(&LocalRay.Origin), LocalOriginXM);
		DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(&LocalRay.Direction), LocalDirXM);

		// 🚨 최적화 2: AABB 검사용 나눗셈(InvDirection)을 1번만 미리 계산 (IEEE 754 0나누기 무한대 성질 이용)
		FVector InvDir(
			1.0f / (LocalRay.Direction.X != 0.0f ? LocalRay.Direction.X : 1e-8f),
			1.0f / (LocalRay.Direction.Y != 0.0f ? LocalRay.Direction.Y : 1e-8f),
			1.0f / (LocalRay.Direction.Z != 0.0f ? LocalRay.Direction.Z : 1e-8f)
		);

		bool bHit = false;
		float ClosestLocalDistance = std::numeric_limits<float>::max();
		FVector BestLocalHitPosition = FVector::ZeroVector;

		// 🚨 최적화 3: std::vector 동적 할당 대신 고정 크기 스택 배열 사용
		int32 Stack[64];
		int32 StackPtr = 0;
		Stack[StackPtr++] = 0; // 루트 노드(0번) 푸시

		while (StackPtr > 0)
		{
			int32 NodeIndex = Stack[--StackPtr];
			const FBVHMeshNode& Node = Nodes[NodeIndex];

			if (Node.IsLeaf())
			{
				const TArray<Triangle>& Triangles = BVH.GetTriangles();
				for (int32 i = Node.startIndex; i < Node.endIndex; ++i)
				{
					const Triangle& Tri = Triangles[i];
					float HitDistance = 0.0f;
					FVector HitPosition = FVector::ZeroVector;

					if (IntersectRayTriangle(LocalRay, Tri.Vertex1, Tri.Vertex2, Tri.Vertex3, HitDistance, HitPosition))
					{
						// 🔥 여기서 ClosestLocalDistance가 줄어들면, 
						// 이후의 AABB 검사에서 더 먼 박스들은 즉시 Culling 됩니다.
						if (HitDistance < ClosestLocalDistance)
						{
							ClosestLocalDistance = HitDistance;
							BestLocalHitPosition = HitPosition;
							bHit = true;
						}
					}
				}
			}
			else
			{
				// 자식 노드가 둘 다 있을 경우의 처리 (Front-to-Back Ordering)
				float DistL = std::numeric_limits<float>::max();
				float DistR = std::numeric_limits<float>::max();

				bool bHitL = Node.LeftChild != -1 && IntersectRayAabbFast(LocalRay, InvDir, Nodes[Node.LeftChild].Bounds.Min, Nodes[Node.LeftChild].Bounds.Max, ClosestLocalDistance, DistL);
				bool bHitR = Node.RightChild != -1 && IntersectRayAabbFast(LocalRay, InvDir, Nodes[Node.RightChild].Bounds.Min, Nodes[Node.RightChild].Bounds.Max, ClosestLocalDistance, DistR);

				if (bHitL && bHitR)
				{
					// 거리가 더 먼 쪽을 먼저 Stack에 넣음 (나중에 꺼내게 됨)
					if (DistL > DistR)
					{
						Stack[StackPtr++] = Node.LeftChild;
						Stack[StackPtr++] = Node.RightChild;
					}
					else
					{
						Stack[StackPtr++] = Node.RightChild;
						Stack[StackPtr++] = Node.LeftChild;
					}
				}
				else if (bHitL) Stack[StackPtr++] = Node.LeftChild;
				else if (bHitR) Stack[StackPtr++] = Node.RightChild;
			}
		}
		if (bHit)
		{
			auto BestLocalHitXM = DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&BestLocalHitPosition));
			auto WorldHitXM = DirectX::XMVector3TransformCoord(BestLocalHitXM, XM);
			FVector WorldHitPosition;
			DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(&WorldHitPosition), WorldHitXM);

			const float DistanceSquared = FVector::DistSquared(InRay.Origin, WorldHitPosition);

			if (DistanceSquared < InOutBestHit.DistanceSquared)
			{
				InOutBestHit.DistanceSquared = DistanceSquared;
				InOutBestHit.PrimitiveId = InPrimitiveRuntimeData.PrimitiveId;
				InOutBestHit.WorldPosition = WorldHitPosition;
				return true;
			}
		}

		return false;
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
	//projection inverse matrix
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