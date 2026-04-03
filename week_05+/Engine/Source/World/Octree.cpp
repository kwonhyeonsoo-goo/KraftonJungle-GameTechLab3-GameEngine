#include "Octree.h"
#include "Component/PrimitiveComponent.h"
#include "Component/MeshComponent.h"
#include "Math/Frustum.h"
#include "Debug/EngineLog.h"

bool RayAABBIntersect(const FRay& Ray, const FVector& BoxMin, const FVector& BoxMax, float& OutDistance)
{
	float TEnter = 0.0f;
	float TExit = 100000.0f;

	for (int i = 0; i < 3; ++i)
	{
		float T1 = (BoxMin.XYZ[i] - Ray.Origin.XYZ[i]) * Ray.InvDirection.XYZ[i];
		float T2 = (BoxMax.XYZ[i] - Ray.Origin.XYZ[i]) * Ray.InvDirection.XYZ[i];
		float TMin = std::min(T1, T2);
		float TMax = std::max(T1, T2);

		TEnter = std::max(TEnter, TMin);
		TExit = std::min(TExit, TMax);
	}

	if (TEnter > TExit || TExit < 0.0f)
	{
		return false;
	}

	OutDistance = std::max(0.0f, TEnter);
	return true;
}

bool RayTriangleIntersect(const FRay& Ray, const FVector& V0, const FVector& V1, const FVector& V2, float& OutDistance)
{
	constexpr float Epsilon = 1.e-6f;

	const FVector Edge1 = V1 - V0;
	const FVector Edge2 = V2 - V0;

	const FVector H = FVector::CrossProduct(Ray.Direction, Edge2);
	const float A = FVector::DotProduct(Edge1, H);
	if (A <= Epsilon)
	{
		return false;
	}

	const float F = 1.0f / A;
	const FVector S = Ray.Origin - V0;
	const float U = F * FVector::DotProduct(S, H);
	if (U < 0.0f || U > 1.0f)
	{
		return false;
	}

	const FVector Q = FVector::CrossProduct(S, Edge1);
	const float V = F * FVector::DotProduct(Ray.Direction, Q);
	if (V < 0.0f || U + V > 1.0f)
	{
		return false;
	}

	const float T = F * FVector::DotProduct(Edge2, Q);
	if (T > Epsilon)
	{
		OutDistance = T;
		return true;
	}

	return false;
}

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

void FOctreeNode::Pick(const FRay& Ray, float& OutClosestDist, UPrimitiveComponent*& OutPrimitive) const
{
	float TEnter;
	if (!RayAABBIntersect(Ray, Min, Max, TEnter) || TEnter > OutClosestDist)
	{
		return;
	}

	for (UPrimitiveComponent* Prim : Primitives)
	{
		FBoxSphereBounds Bounds = Prim->GetWorldBounds();
		float PrimDist;
		if (RayAABBIntersect(Ray, Bounds.Center - Bounds.BoxExtent, Bounds.Center + Bounds.BoxExtent, PrimDist))
		{
			if (PrimDist >= OutClosestDist) continue;
			
			FMeshData* Mesh = nullptr;
			if (Prim->GetPrimitive())
			{
				Mesh = Prim->GetPrimitive()->GetMeshData();
			}
			else if (Prim->IsA(UMeshComponent::StaticClass()))
			{
				Mesh = static_cast<UMeshComponent*>(Prim)->GetMeshData();
			}

			if (Mesh && Mesh->Indices.size() >= 3)
			{
				const FMatrix World = Prim->GetWorldTransform();
				bool bHitMesh = false;
				float MeshClosestDist = OutClosestDist;

				for (uint32 Index = 0; Index + 2 < Mesh->Indices.size(); Index += 3)
				{
					FVector V0 = World.TransformPosition(Mesh->Vertices[Mesh->Indices[Index]].Position);
					FVector V1 = World.TransformPosition(Mesh->Vertices[Mesh->Indices[Index + 1]].Position);
					FVector V2 = World.TransformPosition(Mesh->Vertices[Mesh->Indices[Index + 2]].Position);
					float TriDist;
					if (RayTriangleIntersect(Ray, V0, V1, V2, TriDist) && TriDist < OutClosestDist)
					{
						MeshClosestDist = TriDist;
						bHitMesh = true;
					}
				}

				if (bHitMesh && MeshClosestDist < OutClosestDist)
				{
					OutClosestDist = MeshClosestDist;
					OutPrimitive = Prim;
				}
			}
			else
			{
				if (PrimDist < OutClosestDist)
				{
					OutClosestDist = PrimDist;
					OutPrimitive = Prim;
				}
			}
		}
	}

	if (!bIsLeaf)
	{
		for (int i = 0; i < 8; ++i)
		{
			if (Children[i])
			{
				Children[i]->Pick(Ray, OutClosestDist, OutPrimitive);
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

void FOctree::Pick(const FRay& Ray, float& OutClosestDist, UPrimitiveComponent*& OutPrimitive) const
{
	if (!Root) return;

	Root->Pick(Ray, OutClosestDist, OutPrimitive);
}
