#pragma once

#include "EngineAPI.h"
#include "Math/Vector.h"
#include "Math/Frustum.h"
#include "Types/Array.h"
#include "Math/Ray.h"

class UPrimitiveComponent;

struct ENGINE_API FOctreeNode
{
	FVector Min;
	FVector Max;
	FOctreeNode* Children[8];
	TArray<UPrimitiveComponent*> Primitives;
	bool bIsLeaf;

	FOctreeNode() : bIsLeaf(true)
	{
		for (int i = 0; i < 8; ++i) Children[i] = nullptr;
	}

	void Insert(UPrimitiveComponent* InPrimitive, int32 CurrDepth, int32 MaxDepth, int32 Capacity);
	void Subdivide();

	bool IsInside(UPrimitiveComponent* InPrimitive) const;
	bool IsCenterInside(UPrimitiveComponent* InPrimitive) const;

	void GetVisiblePrimitives(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& OutPrimitives) const;
	void GatherAllPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const;

	void Pick(const FRay& Ray, float& OutClosestDist, UPrimitiveComponent*& OutPrimitive) const;
};

class ENGINE_API FOctree
{
public:
	FOctree(const FVector& Center, float HalfExtent, int32 InMaxDepth = 8, int32 InCapacity = 16);
	~FOctree();

	void Insert(UPrimitiveComponent* InPrimitive);
	void GetVisiblePrimitives(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& OutPrimitives) const;

	void Pick(const FRay& Ray, float& OutClosestDist, UPrimitiveComponent*& OutPrimitive) const;

private:
	void ClearNode(FOctreeNode* Node);

private:
	FOctreeNode* Root;
	int32 MaxDepth;
	int32 Capacity;
};
