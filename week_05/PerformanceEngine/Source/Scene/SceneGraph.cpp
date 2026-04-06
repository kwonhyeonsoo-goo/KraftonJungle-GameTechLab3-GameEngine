#include "SceneGraph.h"
#include "Visibility/VisibilitySystem.h"
#include "Scene/Scene.h"
#include "Scene/SceneTypes.h"
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

void FSceneGraph::Reset()
{
    Nodes.clear();
    PrimitiveIndexBuffer.clear();
	RootIndex = -1;
}

void FSceneGraph::Build(const TArray<FBoundingBox>& ObjectBoxes)
{
    // 1. 기존 데이터 초기화
    Nodes.clear();
    PrimitiveIndexBuffer.clear(); // 🚨 방금 새로 만든 인덱스 버퍼도 꼭 비워줍니다!

    if (ObjectBoxes.empty()) return;

    // 2. 전체 씬 AABB 계산 및 초기 인덱스 배열 생성
    FBoundingBox SceneVolume = ObjectBoxes[0];
    TArray<int32> Indices;
    Indices.reserve(ObjectBoxes.size());

    for (int32 i = 0; i < ObjectBoxes.size(); i++)
    {
        SceneVolume.Encapsulate(ObjectBoxes[i]);
        Indices.push_back(i);
    }

    // 3. 루트 노드부터 재귀 빌드 시작
    RootIndex = BuildRecursive(Indices, ObjectBoxes, SceneVolume, 0);
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
void FSceneGraph::Pick(const FRay& InRay, const FVisibilityResults& CandidateVisibilityResults, const TArray<FScenePrimitiveRuntimeData>& PrimitiveBoxes, TArray<int32>& OutCandidates) const
{
    const auto& Visible = CandidateVisibilityResults.VisiblePrimitiveIndices;
    const auto& Flags = CandidateVisibilityResults.VisibleFlags; // 외부에서 온 플래그
    if (Visible.empty()) return;

    OutCandidates.clear();

    float MaxT = std::numeric_limits<float>::max();
    int32 ClosestObjIdx = -1;

    struct FStackNode { int32 Index; float T; };
    FStackNode Stack[128]; // 옥트리 최대 깊이에 맞춰 넉넉하게
    int32 StackPtr = 0;
    Stack[StackPtr++] = { RootIndex, 0.0f };

    alignas(32) float HitDistances[8];

    while (StackPtr > 0)
    {
        // 스택에서 노드 꺼내기
        FStackNode Current = Stack[--StackPtr];

        if (Current.T >= MaxT) break;

		int32 NodeIndex = Current.Index;
        if (NodeIndex < 0 || NodeIndex >= Nodes.size()) continue;
        const FSceneNode& Node = Nodes[NodeIndex];

        // 리프 노드 처리
        if (Node.ChildCount == 0)
        {
            int32 StartIdx = Node.PrimitiveStartIndex;
            int32 EndIdx = StartIdx + Node.PrimitiveCount;

            // 버퍼에서 이 노드에 속한 오브젝트들만 초고속으로 검사
            for (int32 i = StartIdx; i < EndIdx; ++i)
            {
                int32 ObjIdx = PrimitiveIndexBuffer[i];
                if (ObjIdx != -1 && ObjIdx < Flags.size() && Flags[ObjIdx] == 1)
                {
                    float HitT = 0.0f;
                    const FBoundingBox& ObjBox = PrimitiveBoxes[ObjIdx].WorldBounds;

                    if (IntersectRayAabbFast(InRay, InRay.InvDirection, ObjBox.Min, ObjBox.Max, MaxT, HitT))
                    {
                        if (HitT < MaxT)
                        {
                            MaxT = HitT;
                            ClosestObjIdx = ObjIdx;
                        }
                    }
                }
            }
            continue;
        }

        // 🚨 4. AVX 8-Way 동시 검사 (loadu_ps로 정렬 문제 해결)
        uint32 HitMask = IntersectRayAabbAVX(
            InRay, InRay.InvDirection, MaxT,
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
        for (int32 i = HitCount - 1; i >= 0; --i)
        {
            Stack[StackPtr++] = { ChildHits[i].Index, ChildHits[i].T };
        }
    }

    if (ClosestObjIdx != -1)
    {
        OutCandidates.push_back(ClosestObjIdx);
	}
}

int32 FSceneGraph::BuildRecursive(const TArray<int32>& Indices, const TArray<FBoundingBox>& ObjectBoxes, const FBoundingBox& NodeVolume, int32 Depth)
{
    // 그룹 노드 생성
    FSceneNode GroupNode;
    GroupNode.PrimitiveStartIndex = -1;
    GroupNode.Volume = NodeVolume;
    GroupNode.Center = NodeVolume.GetCenter();

    int32 GroupIndex = Nodes.size();
    Nodes.push_back(GroupNode);

    // 트리의 깊이가 얕아져 메모리 점프가 줄어들고 속도가 급상승합니다.
    if (Indices.size() <= 32 || Depth >= 16)
    {
        // 1. 현재 버퍼의 끝부분을 시작점으로 기록
        Nodes[GroupIndex].PrimitiveStartIndex = PrimitiveIndexBuffer.size();
        Nodes[GroupIndex].PrimitiveCount = Indices.size();

        // 2. 인덱스들을 글로벌 버퍼에 차곡차곡 이어 붙임
        for (int32 Idx : Indices)
        {
            PrimitiveIndexBuffer.push_back(Idx);
        }

        Nodes[GroupIndex].ChildCount = 0; // 리프 노드 표시
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

    for (int32 i = 0; i < 8; i++)
    {
        Nodes[GroupIndex].ChildIndices[i] = -1; // -1이면 자식이 없음

        // 절대 광선과 충돌할 수 없는 무한대/음수무한대 박스로 설정 (AVX 검사용)
        Nodes[GroupIndex].ChildMinX[i] = 1e30f; Nodes[GroupIndex].ChildMaxX[i] = -1e30f;
        Nodes[GroupIndex].ChildMinY[i] = 1e30f; Nodes[GroupIndex].ChildMaxY[i] = -1e30f;
        Nodes[GroupIndex].ChildMinZ[i] = 1e30f; Nodes[GroupIndex].ChildMaxZ[i] = -1e30f;
    }
    int32 ValidChildCount = 0;
    // 비어있지 않은 자식만 재귀
    for (int32 i = 0; i < 8; i++)
    {
        if (ChildIndices[i].empty()) continue;

        // 타이트한 AABB 재계산할 때도 ObjectBoxes 사용!
        FBoundingBox TightVolume = ObjectBoxes[ChildIndices[i][0]]; // 🚨 여기 수정
        for (int32 k = 1; k < ChildIndices[i].size(); ++k)
        {
            TightVolume.Encapsulate(ObjectBoxes[ChildIndices[i][k]]); // 🚨 여기 수정
        }

        // 재귀 호출 시 ObjectBoxes 그대로 전달
        int32 ChildIndex = BuildRecursive(ChildIndices[i], ObjectBoxes, TightVolume, Depth + 1);
        //int32 Cnt = Nodes[GroupIndex].ChildCount;
        //Nodes[GroupIndex].ChildIndices[Cnt] = ChildIndex;

        //// 부모의 SIMD 배열에는 이 "타이트한 AABB"를 등록합니다.
        //Nodes[GroupIndex].ChildMinX[Cnt] = TightVolume.Min.X;
        //Nodes[GroupIndex].ChildMinY[Cnt] = TightVolume.Min.Y;
        //Nodes[GroupIndex].ChildMinZ[Cnt] = TightVolume.Min.Z;
        //Nodes[GroupIndex].ChildMaxX[Cnt] = TightVolume.Max.X;
        //Nodes[GroupIndex].ChildMaxY[Cnt] = TightVolume.Max.Y;
        //Nodes[GroupIndex].ChildMaxZ[Cnt] = TightVolume.Max.Z;

        //Nodes[GroupIndex].ChildCount++;
        //Nodes[ChildIndex].Parent = GroupIndex;

        // ========================================================
        Nodes[GroupIndex].ChildIndices[i] = ChildIndex;

        Nodes[GroupIndex].ChildMinX[i] = TightVolume.Min.X;
        Nodes[GroupIndex].ChildMinY[i] = TightVolume.Min.Y;
        Nodes[GroupIndex].ChildMinZ[i] = TightVolume.Min.Z;
        Nodes[GroupIndex].ChildMaxX[i] = TightVolume.Max.X;
        Nodes[GroupIndex].ChildMaxY[i] = TightVolume.Max.Y;
        Nodes[GroupIndex].ChildMaxZ[i] = TightVolume.Max.Z;

        Nodes[ChildIndex].Parent = GroupIndex;
        ValidChildCount++;
    }
    Nodes[GroupIndex].ChildCount = ValidChildCount;
    return GroupIndex;
}
