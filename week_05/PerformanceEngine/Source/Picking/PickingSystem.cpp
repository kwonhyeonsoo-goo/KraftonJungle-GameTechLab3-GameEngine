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

	__forceinline bool IntersectRayAabbFast(const FRay& InRay, const FVector& InvDir, const FVector& InMin, const FVector& InMax, float MaxDistance, float& OutDistance)
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

		__m256 inv_det = _mm256_div_ps(_mm256_set1_ps(1.0f), det);

		// tvec = orig - A
		__m256 tvec_x = _mm256_sub_ps(orig_x, A_X);
		__m256 tvec_y = _mm256_sub_ps(orig_y, A_Y);
		__m256 tvec_z = _mm256_sub_ps(orig_z, A_Z);

		// u = (tvec . pvec) * inv_det
		__m256 u = _mm256_mul_ps(_mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(tvec_x, pvec_x), _mm256_mul_ps(tvec_y, pvec_y)), _mm256_mul_ps(tvec_z, pvec_z)), inv_det);

		__m256 one = _mm256_set1_ps(1.0f);
		// 0.0 <= u <= 1.0
		__m256 u_mask = _mm256_and_ps(_mm256_cmp_ps(u, zero, _CMP_GE_OQ), _mm256_cmp_ps(u, one, _CMP_LE_OQ));

		// qvec = tvec X edge1
		__m256 qvec_x = _mm256_sub_ps(_mm256_mul_ps(tvec_y, edge1_z), _mm256_mul_ps(tvec_z, edge1_y));
		__m256 qvec_y = _mm256_sub_ps(_mm256_mul_ps(tvec_z, edge1_x), _mm256_mul_ps(tvec_x, edge1_z));
		__m256 qvec_z = _mm256_sub_ps(_mm256_mul_ps(tvec_x, edge1_y), _mm256_mul_ps(tvec_y, edge1_x));

		// v = (dir . qvec) * inv_det
		__m256 v = _mm256_mul_ps(_mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(dir_x, qvec_x), _mm256_mul_ps(dir_y, qvec_y)), _mm256_mul_ps(dir_z, qvec_z)), inv_det);

		// 0.0 <= v && u + v <= 1.0
		__m256 u_plus_v = _mm256_add_ps(u, v);
		__m256 v_mask = _mm256_and_ps(_mm256_cmp_ps(v, zero, _CMP_GE_OQ), _mm256_cmp_ps(u_plus_v, one, _CMP_LE_OQ));

		// t = (edge2 . qvec) * inv_det
		__m256 t = _mm256_mul_ps(_mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(edge2_x, qvec_x), _mm256_mul_ps(edge2_y, qvec_y)), _mm256_mul_ps(edge2_z, qvec_z)), inv_det);

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
		const FRay& Ray, const FVector& InvDir, float MaxDistance,
		const float* MinX, const float* MinY, const float* MinZ,
		const float* MaxX, const float* MaxY, const float* MaxZ,
		float* OutDistances) // 크기 8짜리 배열
	{
		// 1. 광선 데이터를 8개로 복제(Broadcast)
		__m256 ox = _mm256_set1_ps(Ray.Origin.X);
		__m256 oy = _mm256_set1_ps(Ray.Origin.Y);
		__m256 oz = _mm256_set1_ps(Ray.Origin.Z);

		__m256 idx = _mm256_set1_ps(InvDir.X);
		__m256 idy = _mm256_set1_ps(InvDir.Y);
		__m256 idz = _mm256_set1_ps(InvDir.Z);

		// 2. X축 검사
		__m256 tx1 = _mm256_mul_ps(_mm256_sub_ps(_mm256_loadu_ps(MinX), ox), idx);
		__m256 tx2 = _mm256_mul_ps(_mm256_sub_ps(_mm256_loadu_ps(MaxX), ox), idx);
		__m256 tmin = _mm256_min_ps(tx1, tx2);
		__m256 tmax = _mm256_max_ps(tx1, tx2);

		// 3. Y축 검사
		__m256 ty1 = _mm256_mul_ps(_mm256_sub_ps(_mm256_loadu_ps(MinY), oy), idy);
		__m256 ty2 = _mm256_mul_ps(_mm256_sub_ps(_mm256_loadu_ps(MaxY), oy), idy);
		tmin = _mm256_max_ps(tmin, _mm256_min_ps(ty1, ty2));
		tmax = _mm256_min_ps(tmax, _mm256_max_ps(ty1, ty2));

		// 4. Z축 검사
		__m256 tz1 = _mm256_mul_ps(_mm256_sub_ps(_mm256_loadu_ps(MinZ), oz), idz);
		__m256 tz2 = _mm256_mul_ps(_mm256_sub_ps(_mm256_loadu_ps(MaxZ), oz), idz);
		tmin = _mm256_max_ps(tmin, _mm256_min_ps(tz1, tz2));
		tmax = _mm256_min_ps(tmax, _mm256_max_ps(tz1, tz2));

		// 5. 충돌 조건 판별: tmax >= tmin AND tmax > 0 AND tmin < MaxDistance
		__m256 zero = _mm256_setzero_ps();
		__m256 max_dist = _mm256_set1_ps(MaxDistance);

		__m256 mask1 = _mm256_cmp_ps(tmax, tmin, _CMP_GE_OQ);
		__m256 mask2 = _mm256_cmp_ps(tmax, zero, _CMP_GT_OQ);
		__m256 mask3 = _mm256_cmp_ps(tmin, max_dist, _CMP_LT_OQ);
		__m256 hit_mask = _mm256_and_ps(_mm256_and_ps(mask1, mask2), mask3);

		// 결과 거리 저장 (음수면 0으로 보정)
		__m256 out_t = _mm256_max_ps(zero, tmin);
		_mm256_storeu_ps(OutDistances, out_t);

		// 부딪힌 결과만 8비트 정수로 뽑아냄 (예: 1, 3, 4번 자식이 맞았다면 00011010)
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
		int32 Stack[64];
		int32 StackPtr = 0;
		Stack[StackPtr++] = 0; // 8-Way 루트 노드(0번) 푸시

		alignas(32) float HitDistances[8];
		const Triangle* TrianglesPtr = BVH.GetTriangles().data();
		// 만약 GetNodes8Way() 접근자 이름이 다르다면 맞춰서 수정해주세요
		const FBVHMeshNode8* NodesPtr = BVH.GetNodes8Way().data(); 

		while (StackPtr > 0)
		{
			int32 NodeIndex = Stack[--StackPtr];
			const FBVHMeshNode8& Node = NodesPtr[NodeIndex];

			// 8명의 자식을 단일 AVX 명령어로 교차 검사
			uint32 HitMask = IntersectRayAabbAVX(
				LocalRay, LocalRay.InvDirection, ClosestLocalDistance,
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
					int32 TriStart = ChildHits[i].TriStart;
					int32 TriCount = ChildHits[i].TriCount;
					int32 BlockCount = TriCount / 8;
					int32 Remainder = TriCount % 8;

					alignas(32) float HitDistancesTri[8];

					for (int32 b = 0; b < BlockCount + (Remainder > 0 ? 1 : 0); ++b)
					{
						int32 ProcessCount = (b == BlockCount) ? Remainder : 8;

						alignas(32) float AX[8], AY[8], AZ[8];
						alignas(32) float BX[8], BY[8], BZ[8];
						alignas(32) float CX[8], CY[8], CZ[8];

						for (int32 k = 0; k < ProcessCount; ++k)
						{
							const Triangle& Tri = TrianglesPtr[TriStart + b * 8 + k];
							AX[k] = Tri.Vertex1.X; AY[k] = Tri.Vertex1.Y; AZ[k] = Tri.Vertex1.Z;
							BX[k] = Tri.Vertex2.X; BY[k] = Tri.Vertex2.Y; BZ[k] = Tri.Vertex2.Z;
							CX[k] = Tri.Vertex3.X; CY[k] = Tri.Vertex3.Y; CZ[k] = Tri.Vertex3.Z;
						}
						// 미사용 칸은 0 처리
						for (int32 k = ProcessCount; k < 8; ++k)
						{
							AX[k] = AY[k] = AZ[k] = BX[k] = BY[k] = BZ[k] = CX[k] = CY[k] = CZ[k] = 0.0f;
						}

						uint32 HitMask = IntersectRayTriangleAVX8(
							LocalRay,
							_mm256_loadu_ps(AX), _mm256_loadu_ps(AY), _mm256_loadu_ps(AZ),
							_mm256_loadu_ps(BX), _mm256_loadu_ps(BY), _mm256_loadu_ps(BZ),
							_mm256_loadu_ps(CX), _mm256_loadu_ps(CY), _mm256_loadu_ps(CZ),
							HitDistancesTri);

						HitMask &= ((1 << ProcessCount) - 1);

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
					Stack[StackPtr++] = ChildHits[i].Index;
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

	struct FStackNode { int32 Index; float Dist; };
	FStackNode Stack[128];
	int32 StackPtr = 0;
	Stack[StackPtr++] = { InSceneGraph.GetRootIndex(), 0.0f };

	float MaxDistance = std::numeric_limits<float>::max();
	alignas(32) float HitDistances[8];

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
			PickRay, PickRay.InvDirection, MaxDistance,
			Node.ChildMinX, Node.ChildMinY, Node.ChildMinZ,
			Node.ChildMaxX, Node.ChildMaxY, Node.ChildMaxZ,
			HitDistances);
		HitMask &= ((1 << Node.ChildCount) - 1);
		if (HitMask == 0) continue;

		struct ChildHit { int32 Index; float Dist; };
		ChildHit ChildHits[8];
		int32 HitCount = 0;

		unsigned long BitIndex;
		while (_BitScanForward(&BitIndex, HitMask))
		{
			HitMask &= ~(1 << BitIndex);
			ChildHits[HitCount++] = { Node.ChildIndices[BitIndex], HitDistances[BitIndex] };
		}

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
			Stack[StackPtr++] = { ChildHits[i].Index, ChildHits[i].Dist };
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