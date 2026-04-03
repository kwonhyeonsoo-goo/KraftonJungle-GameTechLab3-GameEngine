#include "Octree.h"
#include "Component/PrimitiveComponent.h"
#include "Math/Frustum.h"

void FOctreeNode::Insert(UPrimitiveComponent* InPrimitive, int32 CurrDepth, int32 MaxDepth, int32 Capacity)
{
	if (!IsInside(InPrimitive)) return;

	// 리프 노드가 아니라면 재귀적으로 자식 노드를 탐색해서 Insert
	if (!bIsLeaf)
	{
		bool bFoundChild = false;
		for (int i = 0; i < 8; ++i)
		{
			if (Children[i]->IsInside(InPrimitive))
			{
				Children[i]->Insert(InPrimitive, CurrDepth + 1, MaxDepth, Capacity);
				bFoundChild = true;
				break;
			}
		}

		if (!bFoundChild)
		{
			Primitives.push_back(InPrimitive);
		}
		return;
	}

	// 리프 노드라면 추가하고, 용량 확인 후 분할 여부 결정
	Primitives.push_back(InPrimitive);

	if (Primitives.size() > Capacity && CurrDepth < MaxDepth)
	{
		Subdivide();

		TArray<UPrimitiveComponent*> OldPrimitives = Primitives;
		Primitives.clear();

		for (auto& Prim : OldPrimitives)
		{
			// 분할 이후 생긴 자식 노드에 재삽입
			Insert(Prim, CurrDepth, MaxDepth, Capacity);
		}
	}
}

void FOctreeNode::Subdivide()
{
	FVector Center = (Min + Max) * 0.5f;
	FVector HalfSize = (Max - Min) * 0.5f;
	FVector QuarterSize = HalfSize * 0.5f;

	for (int i = 0; i < 8; ++i)
	{
		Children[i] = new FOctreeNode();
		Children[i]->bIsLeaf = true;

		// 비트 플래그를 통한 8분면 결정
		// i: 0(000) ~ 7(111)
		FVector Offset;
		Offset.X = (i & 0b001) ? QuarterSize.X : -QuarterSize.X;
		Offset.Y = (i & 0b010) ? QuarterSize.Y : -QuarterSize.Y;
		Offset.Z = (i & 0b100) ? QuarterSize.Z : -QuarterSize.Z;

		FVector ChildCenter = Center + Offset;
		Children[i]->Min = ChildCenter - QuarterSize;
		Children[i]->Max = ChildCenter + QuarterSize;
	}

	bIsLeaf = false;
}

bool FOctreeNode::IsInside(UPrimitiveComponent* InPrimitive) const
{
	FBoxSphereBounds BoundingBox = InPrimitive->GetWorldBounds();
	FVector PrimMin = BoundingBox.Center - BoundingBox.BoxExtent;
	FVector PrimMax = BoundingBox.Center + BoundingBox.BoxExtent;
	FVector PrimCenter = BoundingBox.Center;

	return (PrimMin.X >= Min.X && PrimMax.X <= Max.X) &&
		(PrimMin.Y >= Min.Y && PrimMax.Y <= Max.Y) &&
		(PrimMin.Z >= Min.Z && PrimMax.Z <= Max.Z);

}

bool FOctreeNode::IsCenterInside(UPrimitiveComponent* InPrimitive) const
{
	FBoxSphereBounds BoundingBox = InPrimitive->GetWorldBounds();
	FVector PrimCenter = BoundingBox.Center;
	return (PrimCenter.X >= Min.X && PrimCenter.X <= Max.X) &&
		(PrimCenter.Y >= Min.Y && PrimCenter.Y <= Max.Y) &&
		(PrimCenter.Z >= Min.Z && PrimCenter.Z <= Max.Z);
}

void FOctreeNode::GetVisiblePrimitives(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	FBoxSphereBounds NodeBounds;

	EOverlapResult Result = Frustum.IntersectBox(Min, Max);

	if (Result == EOverlapResult::Outside) return;

	if (Result == EOverlapResult::Inside)
	{
		GatherAllPrimitives(OutPrimitives);
		return;
	}

	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (Frustum.IsVisible(Prim->GetWorldBounds()))
		{
			OutPrimitives.push_back(Prim);
		}
	}

	if (!bIsLeaf)
	{
		for (int i = 0; i < 8; ++i)
		{
			if (Children[i])
			{
				Children[i]->GetVisiblePrimitives(Frustum, OutPrimitives);
			}
		}
	}
}

void FOctreeNode::GatherAllPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	OutPrimitives.insert(OutPrimitives.end(), Primitives.begin(), Primitives.end());

	if (!bIsLeaf)
	{
		for (int i = 0; i < 8; ++i)
		{
			if (Children[i])
			{
				Children[i]->GatherAllPrimitives(OutPrimitives);
			}
		}
	}
}


FOctree::FOctree(const FVector& Center, float HalfExtent, int32 InMaxDepth, int32 InCapacity)
	: Root(new FOctreeNode()), MaxDepth(InMaxDepth), Capacity(InCapacity)
{
	Root->Min = Center - FVector(HalfExtent, HalfExtent, HalfExtent);
	Root->Max = Center + FVector(HalfExtent, HalfExtent, HalfExtent);
}

FOctree::~FOctree()
{
	ClearNode(Root);
	delete Root;
}

void FOctree::Insert(UPrimitiveComponent* InPrimitive)
{
	if (Root && Root->IsInside(InPrimitive))
	{
		Root->Insert(InPrimitive, 0, MaxDepth, Capacity);
	}
}

void FOctree::GetVisiblePrimitives(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	if (Root)
	{
		Root->GetVisiblePrimitives(Frustum, OutPrimitives);
	}
}

void FOctree::ClearNode(FOctreeNode* Node)
{
	if (!Node) return;
	for (int i = 0; i < 8; ++i)
	{
		if (Node->Children[i])
		{
			ClearNode(Node->Children[i]);
			delete Node->Children[i];
			Node->Children[i] = nullptr;
		}
	}
	Node->Primitives.clear();
}
