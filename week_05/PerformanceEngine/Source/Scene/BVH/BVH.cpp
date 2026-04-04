#include "BVH.h"
#include <algorithm>

void FBVH::Build(const TArray<FBoundingBox>& ObjectBoxes)
{
	TArray<FBVHObjectInfo> ObjectInfos;
	for (int32 i = 0; i < ObjectInfos.size(); ++i)
	{
		ObjectInfos.push_back({ i, ObjectBoxes[i].GetCenter(), ObjectBoxes[i] });
	}

	Nodes.clear();
	OrderedIndices.clear();

	BuildRecursive(ObjectInfos, 0, static_cast<int32>(ObjectInfos.size()));
}

int32 FBVH::BuildRecursive(TArray<FBVHObjectInfo>& Infos, int32 Start, int32 End)
{
	Nodes.push_back(FBVHNode());
	int32 NodeIndex = Nodes.size();

	FBoundingBox NodeBounds;
	for (int32 i = Start; i < End; ++i)
	{
		NodeBounds.Encapsulate(Infos[i].Box);
	}

	int32 Count = End - Start;
	if (Count <= 8)
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

	int32 Axis = static_cast<float>(NodeBounds.GetLongestAxis());

	int32 Mid = (Start + End) / 2;
	std::nth_element(&Infos[Start], &Infos[Mid], &Infos[End - 1] + 1,
		[Axis](const FBVHObjectInfo& a, const FBVHObjectInfo& b)
		{
			return a.Center[Axis] < b.Center[Axis];
		});

	Nodes[NodeIndex].Bounds = NodeBounds;
	Nodes[NodeIndex].LeftChild = BuildRecursive(Infos, Start, Mid);
	Nodes[NodeIndex].RightChild = BuildRecursive(Infos, Mid, End);

	return NodeIndex;
}
