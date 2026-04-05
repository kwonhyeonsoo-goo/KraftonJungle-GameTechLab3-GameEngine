#include "SceneGraph.h"
#include "Visibility/VisibilitySystem.h"
#include "Scene/Scene.h"
#include "Picking/PickingSystem.h"
#include <limits>

namespace {

    inline bool IntersectRayAabbFast(const FRay& InRay, const FVector& InvDir, const FVector& InMin, const FVector& InMax, float MaxDistance, float& OutDistance)
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

}
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

void FSceneGraph::PickRecursive(int32 NodeIndex, const FRay& InRay, const TArray<int32>& Candidate, TArray<int32>& OutCandidates, int32& InOutMaxT) const
{
    if (NodeIndex == -1) return;

    const FSceneNode& Node = Nodes[NodeIndex];
    float HitT;
    auto DistanceSq3D = [](const FVector& A, const FVector& B)
        {
            float dx = A.X - B.X;
            float dy = A.Y - B.Y;
            float dz = A.Z - B.Z;
            return dx * dx + dy * dy + dz * dz;
        };

    if (!IntersectRayAabbFast(InRay, InRay.Direction, Node.Volume.Min, Node.Volume.Max, InOutMaxT, HitT))
        return;  // t >= InOutMaxT면 자동 컬링

    if (Node.Children.empty()) {
        if (Node.PrimitiveIndex != -1) {
            if (std::find(Candidate.begin(), Candidate.end(), Node.PrimitiveIndex) != Candidate.end())
                OutCandidates.push_back(Node.PrimitiveIndex);
        }
        return;
    }

    for (int32 ChildIndex : Node.Children) {
        PickRecursive(ChildIndex, InRay, Candidate, OutCandidates, InOutMaxT);
    }
}

int32 FSceneGraph::BuildRecursive(const TArray<int32>& Indices, const FBoundingBox& NodeVolume, int32 Depth)
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
