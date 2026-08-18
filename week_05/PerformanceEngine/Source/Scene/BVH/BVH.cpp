#include "BVH.h"
#include <algorithm>

void FBVH::Reset()
{
	Nodes.clear();
	OrderedIndices.clear();
}

void FBVH::Build(const TArray<FBoundingBox>& ObjectBoxes)
{
	TArray<FBVHObjectInfo> ObjectInfos;
	ObjectInfos.reserve(ObjectBoxes.size());
	for (int32 i = 0; i < ObjectBoxes.size(); ++i)
	{
		ObjectInfos.push_back({ i, ObjectBoxes[i].GetCenter(), ObjectBoxes[i] });
	}

	Nodes.clear();
	OrderedIndices.clear();

	BuildRecursive(ObjectInfos, 0, static_cast<int32>(ObjectInfos.size()));
}

int32 FBVH::BuildRecursive(TArray<FBVHObjectInfo>& Infos, int32 Start, int32 End)
{
	int32 NodeIndex = static_cast<int32>(Nodes.size());
	Nodes.push_back(FBVHNode());

	FBoundingBox NodeBounds;
	for (int32 i = Start; i < End; ++i)
	{
		NodeBounds.Encapsulate(Infos[i].Box);
	}

	int32 Count = End - Start;
	if (Count <= 16)
	{
		Nodes[NodeIndex].Bounds = NodeBounds;
		Nodes[NodeIndex].ObjectIndicesStart = static_cast<int32>(OrderedIndices.size());
		Nodes[NodeIndex].ObjectCount = Count;
		for (int32 i = Start; i < End; ++i)
		{
			OrderedIndices.push_back(Infos[i].ObjectIndex);
		}
		return NodeIndex;
	}

	int32 Axis = NodeBounds.GetLongestAxis();
	int32 Mid = (Start + End) / 2;

	std::nth_element(&Infos[Start], &Infos[Mid], &Infos[End - 1] + 1,
		[Axis](const FBVHObjectInfo& a, const FBVHObjectInfo& b)
		{
			return a.Center[Axis] < b.Center[Axis];
		});

	Nodes[NodeIndex].Bounds = NodeBounds;

	int32 Left = BuildRecursive(Infos, Start, Mid);
	int32 Right = BuildRecursive(Infos, Mid, End);

	Nodes[NodeIndex].LeftChild = Left;
	Nodes[NodeIndex].RightChild = Right;

	return NodeIndex;
}

void FBVH::GetVisibleObjects(const FFrustum& InFrustum, const FVector& CameraPos, TArray<int32>& OutVisibleObjectIndices) const
{
	if (Nodes.empty()) return;

	//TArray<int32> NodeStack;
	//NodeStack.push_back(0);
	int32 NodeStack[64];
	int32 StackPtr = 0;
	NodeStack[StackPtr++] = 0; // Root 노드 Push

	while (StackPtr > 0)
	{
		int32 CurrentIndex = NodeStack[--StackPtr];
		const FBVHNode& Node = Nodes[CurrentIndex];

		if (InFrustum.IsOutSide(Node.Bounds)) continue;

		if (Node.IsLeaf())
		{
			for (int32 i = 0; i < Node.ObjectCount; ++i)
			{
				OutVisibleObjectIndices.push_back(OrderedIndices[Node.ObjectIndicesStart + i]);
			}
			continue;
		}

		//float DistSqL = FVector::DistSquared(CameraPos, Nodes[Node.LeftChild].Bounds.GetCenter());
		//float DistSqR = FVector::DistSquared(CameraPos, Nodes[Node.RightChild].Bounds.GetCenter());

		if (StackPtr < 62)
		{
			NodeStack[StackPtr++] = Node.LeftChild;
			NodeStack[StackPtr++] = Node.RightChild;
		}
	}
}
