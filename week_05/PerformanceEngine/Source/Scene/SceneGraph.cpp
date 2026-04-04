#include "SceneGraph.h"
#include "Visibility/VisibilitySystem.h"
#include "Scene/Scene.h"
#include "Picking/PickingSystem.h"
#include <limits>
void FSceneGraph::Build(const TArray<FBoundingBox>& ObjectBoxes)
{
    Nodes.clear();

    // 전체 씬 AABB 계산
    FBoundingBox SceneVolume;
    TArray<int32> Indices;
    for (int32 i = 0; i < ObjectBoxes.size(); i++)
    {
        FSceneNode Leaf;
        Leaf.PrimitiveIndex = i;
        Leaf.Volume = ObjectBoxes[i];
        Leaf.Center = ObjectBoxes[i].GetCenter();
        Nodes.push_back(Leaf);

        SceneVolume.Encapsulate(ObjectBoxes[i]);
        Indices.push_back(i);
    }

    RootIndex = BuildRecursive(Indices, SceneVolume, 0);
}

void FSceneGraph::Build(const FScene& InScene)
{
    TArray<FScenePrimitiveRuntimeData> Primitives = InScene.GetPrimitiveRuntimeData();
    TArray<FBoundingBox> PrimitiveBoxes;
    PrimitiveBoxes.reserve(Primitives.size());

    for (const FScenePrimitiveRuntimeData& Primitive : Primitives)
    {
        PrimitiveBoxes.push_back(Primitive.WorldBounds);
    }

    Build(PrimitiveBoxes);
}

void FSceneGraph::Pick(const FRay& InRay, const FVisibilityResults& CandidateVisibilityResults, TArray<int32>& OutCandidates)const
{
    
    // 루트부터 재귀 순회
    int32 max = std::numeric_limits<int32>::max();
    PickRecursive(RootIndex, InRay, CandidateVisibilityResults.VisiblePrimitiveIndices, OutCandidates, max);
}

void FSceneGraph::PickRecursive(int32 NodeIndex, const FRay& InRay, const TArray<int32>& Candidate, TArray<int32>& OutCandidates, int32& MAX) const
{
    if (NodeIndex == -1) return;

    const FSceneNode& Node = Nodes[NodeIndex];

    auto DistanceSq3D = [](const FVector& A, const FVector& B)
        {
            float dx = A.X - B.X;
            float dy = A.Y - B.Y;
            float dz = A.Z - B.Z;
            return dx * dx + dy * dy + dz * dz;
        };

    // 레이-AABB 교차 테스트, 안 맞으면 자식 전체 스킵
    if (!Node.Volume.IntersectsRay(InRay)) return;
    else if (DistanceSq3D(InRay.Origin, Node.Center) > MAX) {
        return;
    }
    else {
        MAX = DistanceSq3D(InRay.Origin, Node.Center);
    }
    // 리프 노드
    if (Node.Children.empty())
    {
        if (Node.PrimitiveIndex != -1)
        {
            auto It = std::find(Candidate.begin(), Candidate.end(), Node.PrimitiveIndex);
            if (It != Candidate.end())
                OutCandidates.push_back(Node.PrimitiveIndex);
        }
        return;
    }

    for (int32 ChildIndex : Node.Children) {
        int32 max = std::numeric_limits<int32>::max();
        PickRecursive(ChildIndex, InRay, Candidate, OutCandidates, max);
    }
}

int32 FSceneGraph::BuildRecursive(TArray<int32>& Indices, const FBoundingBox& NodeVolume, int32 Depth)
{
    // 그룹 노드 생성
    FSceneNode GroupNode;
    GroupNode.PrimitiveIndex = -1;
    GroupNode.Volume = NodeVolume;
    GroupNode.Center = NodeVolume.GetCenter();

    int32 GroupIndex = Nodes.size();
    Nodes.push_back(GroupNode);

    // 리프 조건: 오브젝트 1개 or 최대 깊이
    if (Indices.size() == 1 || Depth >= 8)
    {
        if (Indices.size() == 1)
            Nodes[GroupIndex].PrimitiveIndex = Indices[0];
        return GroupIndex;
    }

    FVector Mid = NodeVolume.GetCenter();

    // 8개 자식 공간으로 분류
    TArray<int32> ChildIndices[8];
    for (int32 Idx : Indices)
    {
        FVector C = Nodes[Idx].Center;
        int32 Oct = 0;
        if (C.X > Mid.X) Oct |= 1;
        if (C.Y > Mid.Y) Oct |= 2;
        if (C.Z > Mid.Z) Oct |= 4;
        ChildIndices[Oct].push_back(Idx);
    }

    // 비어있지 않은 자식만 재귀
    for (int32 i = 0; i < 8; i++)
    {
        if (ChildIndices[i].empty()) continue;

        // 자식 AABB 계산
        FBoundingBox ChildVolume;
        ChildVolume.Min.X = (i & 1) ? Mid.X : NodeVolume.Min.X;
        ChildVolume.Min.Y = (i & 2) ? Mid.Y : NodeVolume.Min.Y;
        ChildVolume.Min.Z = (i & 4) ? Mid.Z : NodeVolume.Min.Z;
        ChildVolume.Max.X = (i & 1) ? NodeVolume.Max.X : Mid.X;
        ChildVolume.Max.Y = (i & 2) ? NodeVolume.Max.Y : Mid.Y;
        ChildVolume.Max.Z = (i & 4) ? NodeVolume.Max.Z : Mid.Z;

        int32 ChildIndex = BuildRecursive(ChildIndices[i], ChildVolume, Depth + 1);
        Nodes[GroupIndex].Children.push_back(ChildIndex);
        Nodes[ChildIndex].Parent = GroupIndex;
    }

    return GroupIndex;
}
