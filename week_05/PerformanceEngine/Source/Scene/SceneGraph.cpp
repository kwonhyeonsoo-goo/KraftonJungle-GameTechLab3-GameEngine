#include "SceneGraph.h"
#include "Visibility/VisibilitySystem.h"
#include "Scene/Scene.h"

void FSceneGraph::Build(const TArray<FBoundingBox>& ObjectBoxes)
{
    Nodes.clear();

    if (ObjectBoxes.empty())
    {
        RootIndex = -1;
        return;
    }

    FBoundingBox SceneVolume = ObjectBoxes[0];
    TArray<int32> Indices;
    Indices.reserve(ObjectBoxes.size());

    for (int32 i = 0; i < ObjectBoxes.size(); i++)
    {
        SceneVolume.Encapsulate(ObjectBoxes[i]);
        Indices.push_back(i);
    }

    RootIndex = BuildRecursive(Indices, ObjectBoxes, SceneVolume, 0);
}

void FSceneGraph::Build(const FScene& InScene)
{
    const TArray<FScenePrimitiveRuntimeData>& Primitives = InScene.GetPrimitiveRuntimeData();
    if (Primitives.empty())
    {
        Nodes.clear();
        RootIndex = -1;
        return;
    }
    TArray<FBoundingBox> PrimitiveBoxes;
    PrimitiveBoxes.reserve(Primitives.size());
    TArray<int32> Indices;
    Indices.reserve(Primitives.size());

    FBoundingBox SceneVolume = Primitives[0].WorldBounds;
    for (size_t i = 0; i < Primitives.size(); i++)
    {
        PrimitiveBoxes.push_back(Primitives[i].WorldBounds);
        SceneVolume.Encapsulate(Primitives[i].WorldBounds);
        Indices.push_back(static_cast<int32>(i));
    }

    Nodes.clear();
    RootIndex = BuildRecursive(Indices, PrimitiveBoxes, SceneVolume, 0);
}

void FSceneGraph::Pick(const FRay& InRay, const FVisibilityResults& CandidateVisibilityResults, TArray<int32>& OutCandidates)const
{
    
    OutCandidates.clear();
    PickRecursive(RootIndex, InRay, OutCandidates);
}

void FSceneGraph::PickRecursive(int32 NodeIndex, const FRay& InRay, TArray<int32>& OutCandidates) const
{
    if (NodeIndex < 0 || NodeIndex >= Nodes.size()) return;

    const FSceneNode& Node = Nodes[NodeIndex];

    // 레이-AABB 교차 테스트, 안 맞으면 자식 전체 스킵
    if (!Node.Volume.IntersectsRay(InRay)) return;

    // 리프 노드일 경우 보관 중인 모든 오브젝트 반환
    if (Node.Children.empty())
    {
        for (int32 Idx : Node.PrimitiveIndices)
        {
            OutCandidates.push_back(Idx);
        }
        return;
    }

    // 자식 노드 순회
    for (int32 ChildIndex : Node.Children)
    {
        PickRecursive(ChildIndex, InRay, OutCandidates);
    }
}

int32 FSceneGraph::BuildRecursive(const TArray<int32>& Indices, const TArray<FBoundingBox>& ObjectBoxes, const FBoundingBox& NodeVolume, int32 Depth)
{
    FSceneNode GroupNode;
    GroupNode.Volume = NodeVolume;
    GroupNode.Center = NodeVolume.GetCenter();

    int32 GroupIndex = Nodes.size();
    Nodes.push_back(GroupNode);
    // 리프 조건: 오브젝트가 1개 이하이거나 최대 깊이 도달
    if (Indices.size() <= 1 || Depth >= 8)
    {
        Nodes[GroupIndex].PrimitiveIndices = Indices;
        return GroupIndex;
    }

    FVector Mid = NodeVolume.GetCenter();

    // 8개 자식 공간으로 분류
    TArray<int32> ChildIndices[8];
    for (int32 Idx : Indices)
    {
        FVector C = ObjectBoxes[Idx].GetCenter();
        int32 Oct = 0;
        if (C.X > Mid.X) Oct |= 1;
        if (C.Y > Mid.Y) Oct |= 2;
        if (C.Z > Mid.Z) Oct |= 4;
        ChildIndices[Oct].push_back(Idx);
    }
    // 비어있지 않은 자식만 재귀 생성
    for (int32 i = 0; i < 8; i++)
    {
        if (ChildIndices[i].empty()) continue;

        FBoundingBox ChildVolume;
        ChildVolume.Min.X = (i & 1) ? Mid.X : NodeVolume.Min.X;
        ChildVolume.Min.Y = (i & 2) ? Mid.Y : NodeVolume.Min.Y;
        ChildVolume.Min.Z = (i & 4) ? Mid.Z : NodeVolume.Min.Z;
        ChildVolume.Max.X = (i & 1) ? NodeVolume.Max.X : Mid.X;
        ChildVolume.Max.Y = (i & 2) ? NodeVolume.Max.Y : Mid.Y;
        ChildVolume.Max.Z = (i & 4) ? NodeVolume.Max.Z : Mid.Z;

        int32 ChildIndex = BuildRecursive(ChildIndices[i], ObjectBoxes, ChildVolume, Depth + 1);
        Nodes[GroupIndex].Children.push_back(ChildIndex);
        Nodes[ChildIndex].Parent = GroupIndex;
    }

    return GroupIndex;
}
