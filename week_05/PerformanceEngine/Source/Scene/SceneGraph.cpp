#include "SceneGraph.h"
#include "Visibility/VisibilitySystem.h"
#include "Scene/Scene.h"
#include "Picking/PickingSystem.h"
#include <limits>
#include <immintrin.h> // AVX2 지원 필수
#include <intrin.h> /
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
}

void FSceneGraph::Build(const TArray<FBoundingBox>& ObjectBoxes)
{
    Nodes.clear();
    VisibleFlags.resize(ObjectBoxes.size(), 0);
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
void FSceneGraph::Pick(const FRay& InRay, const FVisibilityResults& CandidateVisibilityResults, TArray<int32>& OutCandidates) const
{
    const auto& Visible = CandidateVisibilityResults.VisiblePrimitiveIndices;
    const auto& Flags = CandidateVisibilityResults.VisibleFlags; // 외부에서 온 플래그
    if (Visible.empty()) return;

    //// 1. VisibleFlags 세팅 (추후 VisibilitySystem으로 이관 강력 권장!)
    //int32 MaxIdx = *std::max_element(Visible.begin(), Visible.end());
    //if (MaxIdx >= VisibleFlags.size()) VisibleFlags.resize(MaxIdx + 1, 0);
    //for (int32 Idx : Visible) VisibleFlags[Idx] = 1;

    OutCandidates.reserve(Visible.size());

    // 2. InvDir 계산
    FVector InvDir(
        1.0f / (std::abs(InRay.Direction.X) > 1e-8f ? InRay.Direction.X : 1e-8f),
        1.0f / (std::abs(InRay.Direction.Y) > 1e-8f ? InRay.Direction.Y : 1e-8f),
        1.0f / (std::abs(InRay.Direction.Z) > 1e-8f ? InRay.Direction.Z : 1e-8f)
    );
    float MaxT = std::numeric_limits<float>::max();

    // 🚨 3. 재귀 함수 제거 -> 로컬 스택을 사용한 Iterative 순회
    int32 Stack[128]; // 옥트리 최대 깊이에 맞춰 넉넉하게
    int32 StackPtr = 0;
    Stack[StackPtr++] = RootIndex;

    alignas(32) float HitDistances[8];

    while (StackPtr > 0)
    {
        // 스택에서 노드 꺼내기
        int32 NodeIndex = Stack[--StackPtr];
        if (NodeIndex < 0 || NodeIndex >= Nodes.size()) continue;
        const FSceneNode& Node = Nodes[NodeIndex];

        // 리프 노드 처리
        if (Node.ChildCount == 0)
        {
            int32 Idx = Node.PrimitiveIndex;
            if (Idx != -1 && Idx < Flags.size() && Flags[Idx] == 1)
            {
                OutCandidates.push_back(Idx);
            }
            continue;
        }

        // 🚨 4. AVX 8-Way 동시 검사 (loadu_ps로 정렬 문제 해결)
        uint32 HitMask = IntersectRayAabbAVX(
            InRay, InvDir, MaxT,
            Node.ChildMinX, Node.ChildMinY, Node.ChildMinZ,
            Node.ChildMaxX, Node.ChildMaxY, Node.ChildMaxZ,
            HitDistances
        );
        HitMask &= ((1 << Node.ChildCount) - 1);
        if (HitMask == 0) continue;

        struct ChildHit { int32 Index; float T; };
        ChildHit ChildHits[8];
        int32 HitCount = 0;

        unsigned long BitIndex;
        while (_BitScanForward(&BitIndex, HitMask))
        {
            // 찾은 비트는 끈다
            HitMask &= ~(1 << BitIndex);
            ChildHits[HitCount++] = { Node.ChildIndices[BitIndex], HitDistances[BitIndex] };
        }

        // (Front-to-Back 순회를 위해 T 오름차순 정렬)
        for (int32 i = 1; i < HitCount; ++i)
        {
            ChildHit Key = ChildHits[i];
            int32 j = i - 1;
            while (j >= 0 && ChildHits[j].T > Key.T)
            {
                ChildHits[j + 1] = ChildHits[j];
                j = j - 1;
            }
            ChildHits[j + 1] = Key;
        }

        // 7. 자식들을 스택에 푸시
        // 배열의 역순으로 푸시해야, 꺼낼 때 가장 가까운 자식(Index 0)부터 꺼내게 됩니다!
        for (int32 i = HitCount - 1; i >= 0; --i)
        {
            Stack[StackPtr++] = ChildHits[i].Index;
        }
    }

    // 8. VisibleFlags 정리
    for (int32 Idx : Visible) VisibleFlags[Idx] = 0;
}
void FSceneGraph::PickRecursive(int32 NodeIndex, const FRay& InRay, const FVector& InvDir,
    const std::vector<uint8_t>& VisibleFlags, TArray<int32>& OutCandidates,
    float& InOutMaxT) const
{
    const FSceneNode& Node = Nodes[NodeIndex];
    float HitT;

    // t >= InOutMaxT면 자동 컬링
    if (!IntersectRayAabbFast(InRay, InvDir, Node.Volume.Min, Node.Volume.Max, InOutMaxT, HitT))
        return;

    // 리프 노드 처리
    if (Node.ChildCount == 0)
    {
        int32 Idx = Node.PrimitiveIndex;
        // std::vector<bool>의 느린 비트 연산 대신, 배열 직접 접근으로 초고속 확인
        if (Idx != -1 && Idx < (int32)VisibleFlags.size() && VisibleFlags[Idx] == 1)
        {
            OutCandidates.push_back(Idx);
        }
        return;
    }

    // 자식 노드들 검사
    //struct ChildHit { int32 Index; float T; };
    //ChildHit ChildHits[8];
    //int32 HitCount = 0;

    //for (int32 ChildIndex : Node.Children)
    //{
    //    float ChildT;
    //    const FSceneNode& Child = Nodes[ChildIndex];
    //    if (IntersectRayAabbFast(InRay, InvDir, Child.Volume.Min, Child.Volume.Max, InOutMaxT, ChildT))
    //    {
    //        ChildHits[HitCount++] = { ChildIndex, ChildT };
    //    }
    //}
    // --- PickRecursive 내부 ---

    // 자식 노드 8개 동시 검사!
    alignas(32) float HitDistances[8];
    uint32 HitMask = IntersectRayAabbAVX(
        InRay, InvDir, InOutMaxT,
        Node.ChildMinX, Node.ChildMinY, Node.ChildMinZ,
        Node.ChildMaxX, Node.ChildMaxY, Node.ChildMaxZ,
        HitDistances
    );

    // 맞은 자식이 하나도 없으면 스킵
    if (HitMask == 0) return;

    struct ChildHit { int32 Index; float T; };
    ChildHit ChildHits[8];
    int32 HitCount = 0;

    // 비트마스크를 확인하여 충돌한 자식만 추출
    for (int i = 0; i < Node.ChildCount; ++i)
    {
        if (HitMask & (1 << i))
        {
            ChildHits[HitCount++] = { Node.ChildIndices[i], HitDistances[i] };
        }
    }

    // 삽입 정렬로 Front-to-Back 정렬 후 재귀 (기존 코드와 동일)
    // ...
    // 3. std::sort 제거 및 인라인 삽입 정렬(Insertion Sort) 적용
    // 최대 8개의 요소 정렬은 함수 오버헤드가 없는 삽입 정렬이 std::sort보다 훨씬 빠릅니다.
    for (int32 i = 1; i < HitCount; ++i)
    {
        ChildHit Key = ChildHits[i];
        int32 j = i - 1;
        while (j >= 0 && ChildHits[j].T > Key.T)
        {
            ChildHits[j + 1] = ChildHits[j];
            j = j - 1;
        }
        ChildHits[j + 1] = Key;
    }

    // Front-to-Back 순회
    for (int32 i = 0; i < HitCount; i++)
    {
        PickRecursive(ChildHits[i].Index, InRay, InvDir, VisibleFlags, OutCandidates, InOutMaxT);
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
        int32 Cnt = Nodes[GroupIndex].ChildCount;
        Nodes[GroupIndex].ChildIndices[Cnt] = ChildIndex;

        // 🚨 SIMD 연산을 위해 자식의 AABB 좌표를 부모의 SoA 배열에 등록!
        Nodes[GroupIndex].ChildMinX[Cnt] = ChildVolume.Min.X;
        Nodes[GroupIndex].ChildMinY[Cnt] = ChildVolume.Min.Y;
        Nodes[GroupIndex].ChildMinZ[Cnt] = ChildVolume.Min.Z;
        Nodes[GroupIndex].ChildMaxX[Cnt] = ChildVolume.Max.X;
        Nodes[GroupIndex].ChildMaxY[Cnt] = ChildVolume.Max.Y;
        Nodes[GroupIndex].ChildMaxZ[Cnt] = ChildVolume.Max.Z;

        Nodes[GroupIndex].ChildCount++;
        Nodes[ChildIndex].Parent = GroupIndex;
    }

    return GroupIndex;
}
