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
#include <DirectXMath.h>

namespace
{
	struct FLocalRayAVX
	{
		__m256 OrigX, OrigY, OrigZ;
		__m256 DirX, DirY, DirZ;
		__m256 InvDirX, InvDirY, InvDirZ;

		FLocalRayAVX(const FRay& Ray)
		{
			OrigX = _mm256_set1_ps(Ray.Origin.X);
			OrigY = _mm256_set1_ps(Ray.Origin.Y);
			OrigZ = _mm256_set1_ps(Ray.Origin.Z);

			DirX = _mm256_set1_ps(Ray.Direction.X);
			DirY = _mm256_set1_ps(Ray.Direction.Y);
			DirZ = _mm256_set1_ps(Ray.Direction.Z);

			InvDirX = _mm256_set1_ps(Ray.InvDirection.X);
			InvDirY = _mm256_set1_ps(Ray.InvDirection.Y);
			InvDirZ = _mm256_set1_ps(Ray.InvDirection.Z);
		}
	};

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

	inline uint32 IntersectRayTriangleAVX8(
		const FRay& Ray,
		const __m256& A_X, const __m256& A_Y, const __m256& A_Z,
		const __m256& B_X, const __m256& B_Y, const __m256& B_Z,
		const __m256& C_X, const __m256& C_Y, const __m256& C_Z,
		float* OutDistances)
	{
		__m256 dir_x = _mm256_set1_ps(Ray.Direction.X);
		__m256 dir_y = _mm256_set1_ps(Ray.Direction.Y);
		__m256 dir_z = _mm256_set1_ps(Ray.Direction.Z);

		__m256 orig_x = _mm256_set1_ps(Ray.Origin.X);
		__m256 orig_y = _mm256_set1_ps(Ray.Origin.Y);
		__m256 orig_z = _mm256_set1_ps(Ray.Origin.Z);

		__m256 edge1_x = _mm256_sub_ps(B_X, A_X);
		__m256 edge1_y = _mm256_sub_ps(B_Y, A_Y);
		__m256 edge1_z = _mm256_sub_ps(B_Z, A_Z);

		__m256 edge2_x = _mm256_sub_ps(C_X, A_X);
		__m256 edge2_y = _mm256_sub_ps(C_Y, A_Y);
		__m256 edge2_z = _mm256_sub_ps(C_Z, A_Z);

		// pvec = dir X edge2
		__m256 pvec_x = _mm256_sub_ps(_mm256_mul_ps(dir_y, edge2_z), _mm256_mul_ps(dir_z, edge2_y));
		__m256 pvec_y = _mm256_sub_ps(_mm256_mul_ps(dir_z, edge2_x), _mm256_mul_ps(dir_x, edge2_z));
		__m256 pvec_z = _mm256_sub_ps(_mm256_mul_ps(dir_x, edge2_y), _mm256_mul_ps(dir_y, edge2_x));

		// det = edge1 . pvec
		__m256 det = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(edge1_x, pvec_x), _mm256_mul_ps(edge1_y, pvec_y)), _mm256_mul_ps(edge1_z, pvec_z));

		__m256 epsilon_v = _mm256_set1_ps(1e-8f);
		__m256 zero = _mm256_setzero_ps();
		// det > 1e-8f 만 유효 (Backface culling이 내장됨. 양면 처리 원할경우 fabs(det) 필요)
		__m256 det_mask = _mm256_cmp_ps(det, epsilon_v, _CMP_GT_OQ);

		//__m256 inv_det = _mm256_div_ps(_mm256_set1_ps(1.0f), det);
		__m256 rcp_det = _mm256_rcp_ps(det);

		// tvec = orig - A
		__m256 tvec_x = _mm256_sub_ps(orig_x, A_X);
		__m256 tvec_y = _mm256_sub_ps(orig_y, A_Y);
		__m256 tvec_z = _mm256_sub_ps(orig_z, A_Z);

		// u = (tvec . pvec) * inv_det
		__m256 u = _mm256_mul_ps(tvec_x, pvec_x);
		u = _mm256_fmadd_ps(tvec_y, pvec_y, u);
		u = _mm256_fmadd_ps(tvec_z, pvec_z, u);
		u = _mm256_mul_ps(u, rcp_det);

		__m256 one = _mm256_set1_ps(1.0f);
		// 0.0 <= u <= 1.0
		__m256 u_mask = _mm256_and_ps(_mm256_cmp_ps(u, zero, _CMP_GE_OQ), _mm256_cmp_ps(u, one, _CMP_LE_OQ));

		// qvec = tvec X edge1
		__m256 qvec_x = _mm256_sub_ps(_mm256_mul_ps(tvec_y, edge1_z), _mm256_mul_ps(tvec_z, edge1_y));
		__m256 qvec_y = _mm256_sub_ps(_mm256_mul_ps(tvec_z, edge1_x), _mm256_mul_ps(tvec_x, edge1_z));
		__m256 qvec_z = _mm256_sub_ps(_mm256_mul_ps(tvec_x, edge1_y), _mm256_mul_ps(tvec_y, edge1_x));

		// v = (dir . qvec) * inv_det
		__m256 v = _mm256_mul_ps(_mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(dir_x, qvec_x), _mm256_mul_ps(dir_y, qvec_y)), _mm256_mul_ps(dir_z, qvec_z)), rcp_det);

		// 0.0 <= v && u + v <= 1.0
		__m256 u_plus_v = _mm256_add_ps(u, v);
		__m256 v_mask = _mm256_and_ps(_mm256_cmp_ps(v, zero, _CMP_GE_OQ), _mm256_cmp_ps(u_plus_v, one, _CMP_LE_OQ));

		// t = (edge2 . qvec) * inv_det
		__m256 t = _mm256_mul_ps(_mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(edge2_x, qvec_x), _mm256_mul_ps(edge2_y, qvec_y)), _mm256_mul_ps(edge2_z, qvec_z)), rcp_det);

		// t > 0.0f
		__m256 t_mask = _mm256_cmp_ps(t, zero, _CMP_GT_OQ);

		// 모든 조건 만족 마스크
		__m256 hit_mask = _mm256_and_ps(_mm256_and_ps(_mm256_and_ps(det_mask, u_mask), v_mask), t_mask);

		_mm256_storeu_ps(OutDistances, t);

		return _mm256_movemask_ps(hit_mask);
	}


	// 8개의 상자를 한 번에 검사하고, 광선과 충돌한 상자들의 결과를 반환합니다.
	// 반환값: 하위 8비트가 각각 자식 0~7의 충돌 여부를 나타내는 비트마스크
	inline uint32 IntersectRayAabbAVX(
		const FLocalRayAVX& RayAVX, float MaxDistance,
		const float* MinX, const float* MinY, const float* MinZ,
		const float* MaxX, const float* MaxY, const float* MaxZ,
		float* OutDistances)
	{
		__m256 tx1 = _mm256_mul_ps(_mm256_sub_ps(_mm256_load_ps(MinX), RayAVX.OrigX), RayAVX.InvDirX);
		__m256 tx2 = _mm256_mul_ps(_mm256_sub_ps(_mm256_load_ps(MaxX), RayAVX.OrigX), RayAVX.InvDirX);
		__m256 tmin = _mm256_min_ps(tx1, tx2);
		__m256 tmax = _mm256_max_ps(tx1, tx2);

		__m256 ty1 = _mm256_mul_ps(_mm256_sub_ps(_mm256_load_ps(MinY), RayAVX.OrigY), RayAVX.InvDirY);
		__m256 ty2 = _mm256_mul_ps(_mm256_sub_ps(_mm256_load_ps(MaxY), RayAVX.OrigY), RayAVX.InvDirY);
		tmin = _mm256_max_ps(tmin, _mm256_min_ps(ty1, ty2));
		tmax = _mm256_min_ps(tmax, _mm256_max_ps(ty1, ty2));

		__m256 tz1 = _mm256_mul_ps(_mm256_sub_ps(_mm256_load_ps(MinZ), RayAVX.OrigZ), RayAVX.InvDirZ);
		__m256 tz2 = _mm256_mul_ps(_mm256_sub_ps(_mm256_load_ps(MaxZ), RayAVX.OrigZ), RayAVX.InvDirZ);
		tmin = _mm256_max_ps(tmin, _mm256_min_ps(tz1, tz2));
		tmax = _mm256_min_ps(tmax, _mm256_max_ps(tz1, tz2));

		__m256 zero = _mm256_setzero_ps();
		__m256 max_dist = _mm256_set1_ps(MaxDistance);

		__m256 mask1 = _mm256_cmp_ps(tmax, tmin, _CMP_GE_OQ);
		__m256 mask2 = _mm256_cmp_ps(tmax, zero, _CMP_GT_OQ);
		__m256 mask3 = _mm256_cmp_ps(tmin, max_dist, _CMP_LT_OQ);
		__m256 hit_mask = _mm256_and_ps(_mm256_and_ps(mask1, mask2), mask3);

		_mm256_storeu_ps(OutDistances, _mm256_max_ps(zero, tmin));
		return _mm256_movemask_ps(hit_mask);
	}

	// 2. 삼각형 검사 (미리 계산된 엣지 배열 사용, load_ps 적용)
	inline uint32 IntersectRayTriangleAVX8(
		const FLocalRayAVX& RayAVX,
		const float* AX, const float* AY, const float* AZ,
		const float* E1X, const float* E1Y, const float* E1Z,
		const float* E2X, const float* E2Y, const float* E2Z,
		float* OutDistances)
	{
		__m256 edge1_x = _mm256_load_ps(E1X);
		__m256 edge1_y = _mm256_load_ps(E1Y);
		__m256 edge1_z = _mm256_load_ps(E1Z);

		__m256 edge2_x = _mm256_load_ps(E2X);
		__m256 edge2_y = _mm256_load_ps(E2Y);
		__m256 edge2_z = _mm256_load_ps(E2Z);

		// pvec = dir X edge2
		__m256 pvec_x = _mm256_sub_ps(_mm256_mul_ps(RayAVX.DirY, edge2_z), _mm256_mul_ps(RayAVX.DirZ, edge2_y));
		__m256 pvec_y = _mm256_sub_ps(_mm256_mul_ps(RayAVX.DirZ, edge2_x), _mm256_mul_ps(RayAVX.DirX, edge2_z));
		__m256 pvec_z = _mm256_sub_ps(_mm256_mul_ps(RayAVX.DirX, edge2_y), _mm256_mul_ps(RayAVX.DirY, edge2_x));

		// det = edge1 . pvec
		__m256 det = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(edge1_x, pvec_x), _mm256_mul_ps(edge1_y, pvec_y)), _mm256_mul_ps(edge1_z, pvec_z));

		__m256 epsilon_v = _mm256_set1_ps(1e-8f);
		__m256 zero = _mm256_setzero_ps();
		__m256 det_mask = _mm256_cmp_ps(det, epsilon_v, _CMP_GT_OQ);

		__m256 rcp_det = _mm256_rcp_ps(det);

		// tvec = orig - A
		__m256 tvec_x = _mm256_sub_ps(RayAVX.OrigX, _mm256_load_ps(AX));
		__m256 tvec_y = _mm256_sub_ps(RayAVX.OrigY, _mm256_load_ps(AY));
		__m256 tvec_z = _mm256_sub_ps(RayAVX.OrigZ, _mm256_load_ps(AZ));

		// u = (tvec . pvec) * inv_det
		__m256 u = _mm256_mul_ps(tvec_x, pvec_x);
		u = _mm256_fmadd_ps(tvec_y, pvec_y, u);
		u = _mm256_fmadd_ps(tvec_z, pvec_z, u);
		u = _mm256_mul_ps(u, rcp_det);

		__m256 one = _mm256_set1_ps(1.0f);
		__m256 u_mask = _mm256_and_ps(_mm256_cmp_ps(u, zero, _CMP_GE_OQ), _mm256_cmp_ps(u, one, _CMP_LE_OQ));

		// qvec = tvec X edge1
		__m256 qvec_x = _mm256_sub_ps(_mm256_mul_ps(tvec_y, edge1_z), _mm256_mul_ps(tvec_z, edge1_y));
		__m256 qvec_y = _mm256_sub_ps(_mm256_mul_ps(tvec_z, edge1_x), _mm256_mul_ps(tvec_x, edge1_z));
		__m256 qvec_z = _mm256_sub_ps(_mm256_mul_ps(tvec_x, edge1_y), _mm256_mul_ps(tvec_y, edge1_x));

		// v = (dir . qvec) * inv_det
		__m256 v = _mm256_mul_ps(_mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(RayAVX.DirX, qvec_x), _mm256_mul_ps(RayAVX.DirY, qvec_y)), _mm256_mul_ps(RayAVX.DirZ, qvec_z)), rcp_det);
		__m256 u_plus_v = _mm256_add_ps(u, v);
		__m256 v_mask = _mm256_and_ps(_mm256_cmp_ps(v, zero, _CMP_GE_OQ), _mm256_cmp_ps(u_plus_v, one, _CMP_LE_OQ));

		// t = (edge2 . qvec) * inv_det
		__m256 t = _mm256_mul_ps(_mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(edge2_x, qvec_x), _mm256_mul_ps(edge2_y, qvec_y)), _mm256_mul_ps(edge2_z, qvec_z)), rcp_det);
		__m256 t_mask = _mm256_cmp_ps(t, zero, _CMP_GT_OQ);

		__m256 hit_mask = _mm256_and_ps(_mm256_and_ps(_mm256_and_ps(det_mask, u_mask), v_mask), t_mask);

		_mm256_storeu_ps(OutDistances, t);
		return _mm256_movemask_ps(hit_mask);
	}
	__forceinline bool IntersectRayTriangle(
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

		if (Determinant < 1.e-8f) return false;

		const float InverseDeterminant = 1.0f / Determinant;
		const FVector TVector = InRay.Origin - InA;
		const float U = FVector::DotProduct(TVector, PVector) * InverseDeterminant;
		
		// U가 0~1 범위를 벗어나면 즉시 종료
		if (U < 0.0f || U > 1.0f) return false;

		const FVector QVector = FVector::CrossProduct(TVector, EdgeAB);
		const float V = FVector::DotProduct(InRay.Direction, QVector) * InverseDeterminant;
		
		// V가 0 미만이거나 U+V가 1.0을 초과하면 즉시 종료
		if (V < 0.0f || U + V > 1.0f) return false;

		const float T = FVector::DotProduct(EdgeAC, QVector) * InverseDeterminant;
		if (T <= 0.0f) return false;

		OutDistance = T;
		OutWorldPosition.X = InRay.Origin.X + InRay.Direction.X * T;
		OutWorldPosition.Y = InRay.Origin.Y + InRay.Direction.Y * T;
		OutWorldPosition.Z = InRay.Origin.Z + InRay.Direction.Z * T;
		return true;
	}

	bool IntersectRenderItem(const FRay& InRay, const FScenePrimitiveRuntimeData& InPrimitiveRuntimeData, FPickHit& InOutBestHit)
	{
		FStaticMesh* StaticMesh = InPrimitiveRuntimeData.StaticMesh;
		if (StaticMesh == nullptr || !StaticMesh->IsValid()) return false;

		const FBVHMesh& BVH = StaticMesh->GetBVH();
		const TArray<FBVHMeshNode>& Nodes = BVH.GetNodes();
		if (Nodes.empty()) return false;

		const auto XM = InPrimitiveRuntimeData.WorldMatrix.ToXMMatrix();
		const auto WorldToLocalXM = InPrimitiveRuntimeData.InverseWorldMatrix.ToXMMatrix();

		auto RayOrigin = DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&InRay.Origin));
		auto RayDir = DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&InRay.Direction));

		auto LocalOriginXM = DirectX::XMVector3TransformCoord(RayOrigin, WorldToLocalXM);

		auto LocalDirUnnormXM = DirectX::XMVector3TransformNormal(RayDir, WorldToLocalXM);
		auto LocalDirLengthXM = DirectX::XMVector3Length(LocalDirUnnormXM);
		float LocalDirLength = DirectX::XMVectorGetX(LocalDirLengthXM);
		
		// 최적화: 길이가 거의 0인 경우 방어 및 빠른 Reciprocal 처리
		if (LocalDirLength < 1e-8f) return false;
		
		auto LocalDirXM = DirectX::XMVectorDivide(LocalDirUnnormXM, LocalDirLengthXM);

		FRay LocalRay;
		DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(&LocalRay.Origin), LocalOriginXM);
		DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(&LocalRay.Direction), LocalDirXM);
		
		LocalRay.InvDirection.X = 1.0f / (LocalRay.Direction.X != 0.0f ? LocalRay.Direction.X : 1e-8f);
		LocalRay.InvDirection.Y = 1.0f / (LocalRay.Direction.Y != 0.0f ? LocalRay.Direction.Y : 1e-8f);
		LocalRay.InvDirection.Z = 1.0f / (LocalRay.Direction.Z != 0.0f ? LocalRay.Direction.Z : 1e-8f);

		bool bHit = false;
		float ClosestLocalDistance = std::numeric_limits<float>::max();
		if (InOutBestHit.DistanceSquared < std::numeric_limits<float>::max())
		{
			ClosestLocalDistance = std::sqrt(InOutBestHit.DistanceSquared) / LocalDirLength;
		}

		FVector BestLocalHitPosition = FVector::ZeroVector;

		// 최적화: 1. 방향 벡터의 부호에 따른 사전 로드 (ray-box 교차 최적화용 캐시)
		int32 SignX = LocalRay.InvDirection.X < 0.0f ? 1 : 0;
		int32 SignY = LocalRay.InvDirection.Y < 0.0f ? 1 : 0;
		int32 SignZ = LocalRay.InvDirection.Z < 0.0f ? 1 : 0;

		//최적화: 고정 크기 스택 사용 (캐시 로컬리티)
		struct FMeshStackNode { int32 Index; float Dist; };
		FMeshStackNode Stack[64];
		int32 StackPtr = 0;
		Stack[StackPtr++] = { 0, 0.0f }; // 8-Way 루트 노드(0번) 푸시

		alignas(32) float HitDistances[8];
		const FTriangleBlock8* TriangleBlocksPtr = BVH.GetTriangleBlocks().data();
		// 만약 GetNodes8Way() 접근자 이름이 다르다면 맞춰서 수정해주세요
		const FBVHMeshNode8* NodesPtr = BVH.GetNodes8Way().data(); 
		FLocalRayAVX rayAVX = FLocalRayAVX(LocalRay);
		while (StackPtr > 0)
		{
			FMeshStackNode CurrentNode = Stack[--StackPtr];
			if (CurrentNode.Dist >= ClosestLocalDistance) continue;

			int32 NodeIndex = CurrentNode.Index;
			const FBVHMeshNode8& Node = NodesPtr[NodeIndex];

			// 8명의 자식을 단일 AVX 명령어로 교차 검사
			uint32 HitMask = IntersectRayAabbAVX(
				rayAVX, ClosestLocalDistance,
				Node.ChildMinX, Node.ChildMinY, Node.ChildMinZ,
				Node.ChildMaxX, Node.ChildMaxY, Node.ChildMaxZ,
				HitDistances);

			HitMask &= ((1 << Node.ValidChildCount) - 1);
			if (HitMask == 0) continue;

			struct ChildHit { int32 Index; float Dist; int32 TriStart; int32 TriCount; };
			ChildHit ChildHits[8];
			int32 HitCount = 0;

			unsigned long BitIndex;
			while (_BitScanForward(&BitIndex, HitMask))
			{
				HitMask &= ~(1 << BitIndex);
				ChildHits[HitCount++] = { Node.ChildIndices[BitIndex], HitDistances[BitIndex], Node.TriangleStart[BitIndex], Node.TriangleCount[BitIndex] };
			}

			// Front-to-Back 순회를 위해 거리가 먼 것을 스택에 먼저 넣습니다 (내림차순 정렬)
			for (int32 i = 1; i < HitCount; ++i)
			{
				ChildHit Key = ChildHits[i];
				int32 j = i - 1;
				while (j >= 0 && ChildHits[j].Dist < Key.Dist)
				{
					ChildHits[j + 1] = ChildHits[j];
					--j;
				}
				ChildHits[j + 1] = Key;
			}

			for (int32 i = 0; i < HitCount; ++i)
			{
				if (ChildHits[i].Index == -1) // 리프 노드 (삼각형)
				{
					int32 BlockStart = ChildHits[i].TriStart;
					int32 BlockCount = ChildHits[i].TriCount;

					alignas(32) float HitDistancesTri[8];

					for (int32 b = 0; b < BlockCount; ++b)
					{
						const FTriangleBlock8& Block = TriangleBlocksPtr[BlockStart + b];
						
						uint32 HitMask = IntersectRayTriangleAVX8(
							LocalRay,
							_mm256_load_ps(Block.AX), _mm256_load_ps(Block.AY), _mm256_load_ps(Block.AZ),
							_mm256_load_ps(Block.BX), _mm256_load_ps(Block.BY), _mm256_load_ps(Block.BZ),
							_mm256_load_ps(Block.CX), _mm256_load_ps(Block.CY), _mm256_load_ps(Block.CZ),
							HitDistancesTri);

						HitMask &= ((1 << Block.TriCount) - 1);

						unsigned long BitIndex;
						while (_BitScanForward(&BitIndex, HitMask))
						{
							HitMask &= ~(1 << BitIndex);
							float HitDist = HitDistancesTri[BitIndex];
							if (HitDist < ClosestLocalDistance)
							{
								ClosestLocalDistance = HitDist;
								BestLocalHitPosition.X = LocalRay.Origin.X + LocalRay.Direction.X * HitDist;
								BestLocalHitPosition.Y = LocalRay.Origin.Y + LocalRay.Direction.Y * HitDist;
								BestLocalHitPosition.Z = LocalRay.Origin.Z + LocalRay.Direction.Z * HitDist;
								bHit = true;
							}
						}
					}
				}
				else // 내부 노드
				{
					Stack[StackPtr++] = { ChildHits[i].Index, ChildHits[i].Dist };
				}
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
	Result.InvDirection = FVector(
		1.0f / (Result.Direction.X != 0.0f ? Result.Direction.X : 1e-8f),
		1.0f / (Result.Direction.Y != 0.0f ? Result.Direction.Y : 1e-8f),
		1.0f / (Result.Direction.Z != 0.0f ? Result.Direction.Z : 1e-8f)
	);

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
		Result.InvDirection = FVector(
			1.0f / (Direction.X != 0.0f ? Direction.X : 1e-8f),
			1.0f / (Direction.Y != 0.0f ? Direction.Y : 1e-8f),
			1.0f / (Direction.Z != 0.0f ? Direction.Z : 1e-8f)
		);
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

	const uint64 WorldPickStartCycles = QueryCycles64();

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
	const TArray<FScenePrimitiveRuntimeData>& PrimitiveRuntimeData = InScene.GetPrimitiveRuntimeData();

	FPickHit BestHit;
	BestHit.DistanceSquared = std::numeric_limits<float>::max();
	BestHit.PrimitiveId = -1;

	const auto& Flags = InVisibilityResults.VisibleFlags;
	const auto& Nodes = InSceneGraph.GetNodes();
	const auto& IndexBuffer = InSceneGraph.GetPrimitiveIndexBuffer();

	if (Nodes.empty() || InSceneGraph.GetRootIndex() == -1) return;

	const uint64 WorldPickEndCycles = QueryCycles64();
	const uint64 MeshPickStartCycles = QueryCycles64();

	struct FStackNode { int32 Index; float Dist; };
	FStackNode Stack[128];
	int32 StackPtr = 0;
	Stack[StackPtr++] = { InSceneGraph.GetRootIndex(), 0.0f };

	float MaxDistance = std::numeric_limits<float>::max();
	alignas(32) float HitDistances[8];

	// Ray-Sign Traversal
	int32 RaySignX = PickRay.InvDirection.X < 0.0f ? 1 : 0;
	int32 RaySignY = PickRay.InvDirection.Y < 0.0f ? 1 : 0;
	int32 RaySignZ = PickRay.InvDirection.Z < 0.0f ? 1 : 0;
	int32 RayOctant = RaySignX | (RaySignY << 1) | (RaySignZ << 2);

	static const int32 OctantOrder[8][8] = {
		{0, 1, 2, 3, 4, 5, 6, 7}, // +X, +Y, +Z 방향
		{1, 0, 3, 2, 5, 4, 7, 6}, // -X, +Y, +Z 방향
		{2, 3, 0, 1, 6, 7, 4, 5}, // +X, -Y, +Z 방향
		{3, 2, 1, 0, 7, 6, 5, 4}, // -X, -Y, +Z 방향
		{4, 5, 6, 7, 0, 1, 2, 3}, // +X, +Y, -Z 방향
		{5, 4, 7, 6, 1, 0, 3, 2}, // -X, +Y, -Z 방향
		{6, 7, 4, 5, 2, 3, 0, 1}, // +X, -Y, -Z 방향
		{7, 6, 5, 4, 3, 2, 1, 0}  // -X, -Y, -Z 방향
	};
	const int32* TraversalOrder = OctantOrder[RayOctant];
	FLocalRayAVX rayAVX = FLocalRayAVX(PickRay);
	while (StackPtr > 0)
	{
		FStackNode Current = Stack[--StackPtr];

		if (Current.Dist >= MaxDistance) continue;

		int32 NodeIndex = Current.Index;
		if (NodeIndex < 0 || NodeIndex >= Nodes.size()) continue;
		const FSceneNode& Node = Nodes[NodeIndex];

		if (Node.ChildCount == 0)
		{
			int32 StartIdx = Node.PrimitiveStartIndex;
			int32 EndIdx = StartIdx + Node.PrimitiveCount;

			for (int32 i = StartIdx; i < EndIdx; ++i)
			{
				int32 ObjIdx = IndexBuffer[i];
				if (ObjIdx != -1 && ObjIdx < Flags.size() && Flags[ObjIdx] == 1)
				{
					if (IntersectRenderItem(PickRay, PrimitiveRuntimeData[ObjIdx], BestHit))
					{
						BestHit.PrimitiveIndex = ObjIdx;
						MaxDistance = std::sqrt(BestHit.DistanceSquared);
					}
				}
			}
			continue;
		}

		uint32 HitMask = IntersectRayAabbAVX(
			rayAVX, MaxDistance,
			Node.ChildMinX, Node.ChildMinY, Node.ChildMinZ,
			Node.ChildMaxX, Node.ChildMaxY, Node.ChildMaxZ,
			HitDistances);
		HitMask &= ((1 << Node.ChildCount) - 1);
		if (HitMask == 0) continue;

		// 족보 순서대로 뒤에서부터(가장 먼 것부터) 스택에 푸시
		for (int32 i = 7; i >= 0; --i)
		{
			int32 ChildIdx = TraversalOrder[i]; // 이번에 확인할 자식 인덱스

			// 해당 자식이 광선과 부딪혔는지 마스크로 확인
			if (HitMask & (1 << ChildIdx))
			{
				// 부딪혔다면 스택에 푸시
				Stack[StackPtr++] = { Node.ChildIndices[ChildIdx], HitDistances[ChildIdx] };
			}
		}
	}
	const uint64 MeshPickEndCycles = QueryCycles64();
	const uint64 PickEndCycles = QueryCycles64();
	InOutPickState.LastPickTimeMs = CyclesToMilliseconds(PickStartCycles, PickEndCycles);
	InOutPickState.LastWorldPickTimeMs = CyclesToMilliseconds(WorldPickStartCycles, WorldPickEndCycles);
	InOutPickState.LastMeshPickTimeMS = CyclesToMilliseconds(MeshPickStartCycles, MeshPickEndCycles);
	InOutPickState.TotalPickTimeMs += InOutPickState.LastPickTimeMs;
	++InOutPickState.TotalPickCount;

	// 기즈모를 누르지 않았을 때만 씬 오브젝트 선택 결과를 갱신합니다.
	InOutPickState.bHit = BestHit.PrimitiveId >= 0;
	InOutPickState.SelectedPrimitiveId = BestHit.PrimitiveId;
	InOutPickState.SelectedPrimitiveIndex = BestHit.PrimitiveIndex;
	InOutPickState.HitWorldPosition = BestHit.WorldPosition;
}