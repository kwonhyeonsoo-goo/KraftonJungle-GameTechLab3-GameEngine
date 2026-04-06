#pragma once

#include <limits>
#include <memory>

#include "Math/BoundingBox.h"
#include "Math/Matrix.h"
#include "Math/Transform.h"
#include "Types/PlatformTypes.h"
#include "Types/String.h"
#include "StaticMesh/BVH/BVHMesh.h"
#include "StaticMesh/StaticMesh.h"
class FStaticMesh;



struct FPickHit
{
	int32 PrimitiveId = -1;
	int32 PrimitiveIndex = -1;
	float DistanceSquared = std::numeric_limits<float>::max();
	FVector WorldPosition = FVector::ZeroVector;
};

struct FSceneCameraInitData
{
	FTransform Transform = FTransform::Identity;
	float FovDegrees = 60.0f;
	float NearClip = 0.1f;
	float FarClip = 1000.0f;
};

struct FScenePrimitiveColdData
{
	FString MeshAssetPath;
	std::shared_ptr<FStaticMesh> StaticMeshOwner;
};
struct FInstanceData
{
	DirectX::XMFLOAT4X4 WorldMatrix;
	DirectX::XMFLOAT3 Center;
	float Padding1 = 0.0f;
	DirectX::XMFLOAT3 Extents;
	float Padding2 = 0.0f;
};
#include "SceneComponent.h"
struct FScenePrimitiveRuntimeData : public FSceneComponent
{
	int32 PrimitiveId = -1;
	FMatrix WorldMatrix = FMatrix::Identity;
	FMatrix InverseWorldMatrix = FMatrix::Identity;
	FBoundingBox WorldBounds;
	FStaticMesh* StaticMesh = nullptr;
	FInstanceData CachedInstanceData;
	virtual void OnUpdateWorldTransform() override
	{
		if (StaticMesh)
		{
			FVector LocalMin = StaticMesh->GetBoundsMin();
			FVector LocalMax = StaticMesh->GetBoundsMax();

			const FVector Corners[8] = {
				FVector(LocalMin.X, LocalMin.Y, LocalMin.Z), FVector(LocalMax.X, LocalMin.Y, LocalMin.Z),
				FVector(LocalMin.X, LocalMax.Y, LocalMin.Z), FVector(LocalMax.X, LocalMax.Y, LocalMin.Z),
				FVector(LocalMin.X, LocalMin.Y, LocalMax.Z), FVector(LocalMax.X, LocalMin.Y, LocalMax.Z),
				FVector(LocalMin.X, LocalMax.Y, LocalMax.Z), FVector(LocalMax.X, LocalMax.Y, LocalMax.Z)
			};

			FVector NewMin(FLT_MAX, FLT_MAX, FLT_MAX);
			FVector NewMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

			// 나의 최신 월드 행렬을 가져옴
			const FMatrix& WorldMat = GetComponentToWorld();

			WorldMatrix = WorldMat;
			InverseWorldMatrix = WorldMat.GetInverse();

			for (int i = 0; i < 8; ++i)
			{
				FVector Transformed = WorldMat.TransformPosition(Corners[i]);
				NewMin = FVector::Min(NewMin, Transformed);
				NewMax = FVector::Max(NewMax, Transformed);
			}

			WorldBounds.Min = NewMin;
			WorldBounds.Max = NewMax;
			DirectX::XMStoreFloat4x4(&CachedInstanceData.WorldMatrix, DirectX::XMMatrixTranspose(WorldMat.ToXMMatrix()));
			CachedInstanceData.Center = WorldBounds.GetCenter().ToXMFLOAT3();
			CachedInstanceData.Extents = WorldBounds.GetExtents().ToXMFLOAT3();
		}
	}
};
