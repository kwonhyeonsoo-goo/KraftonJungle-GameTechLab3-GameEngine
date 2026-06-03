#pragma once

#include "Cloth/ClothCollisionTypes.h"
#include "Core/Types/CoreTypes.h"
#include "Math/Vector.h"
#include "Profiling/Stats/Stats.h"

struct FClothCollisionStatCandidate
{
    uint32 OwnerActorId = 0;
    uint32 OwnerComponentId = 0;
    int32 BodyIndex = -1;
    int32 ShapeIndex = -1;
    EClothCollisionPrimitiveType Type = EClothCollisionPrimitiveType::Sphere;
    EClothCollisionSelectState State = EClothCollisionSelectState::Gathered;
    FVector BoundsCenter = FVector::ZeroVector;
    FVector BoundsExtent = FVector::ZeroVector;
};

struct FClothCollisionStats
{
    static uint32 TickAttempts;
    static uint32 SkippedNoAsset;
    static uint32 SkippedNoClothPayload;
    static uint32 SkippedNonCPUSkinning;
    static uint32 SkippedNoSkinnedVertices;
    static uint32 ComponentCount;
    static uint32 EnabledClothSections;
    static uint32 CollisionEligibleSections;
    static uint32 WorldStaticConfiguredSections;
    static uint32 InvalidSectionBounds;
    static uint32 MissingPhysicsRuntimeSections;
    static uint32 SectionCount;
    static uint32 WorldStaticEnabledSections;
    static uint32 WorldStaticCandidateCount;
    static uint32 WorldStaticGatheredSpheres;
    static uint32 WorldStaticGatheredCapsules;
    static uint32 WorldStaticGatheredBoxes;
    static uint32 WorldStaticSelectedSpheres;
    static uint32 WorldStaticSelectedCapsules;
    static uint32 WorldStaticSelectedBoxes;
    static uint32 WorldStaticRejectedBySectionBounds;
    static uint32 WorldStaticSkippedFilter;
    static FClothCollisionDebugStats DebugStats;
    static TArray<FClothCollisionStatCandidate> RecentWorldStaticCandidates;

    static void Reset();
    static void AddTickAttempt();
    static void AddSkippedNoAsset();
    static void AddSkippedNoClothPayload();
    static void AddSkippedNonCPUSkinning();
    static void AddSkippedNoSkinnedVertices();
    static void AddEnabledSection();
    static void AddCollisionEligibleSection(bool bWorldStaticConfigured, bool bHasValidBounds, bool bHasPhysicsRuntime);
    static void AddComponentResult();
    static void AddSectionResult(const FClothCollisionGatherResult& Result, bool bWorldStaticEnabled);
};

#if STATS
#define CLOTH_COLLISION_STATS_RESET() FClothCollisionStats::Reset()
#define CLOTH_COLLISION_STATS_ADD_TICK_ATTEMPT() FClothCollisionStats::AddTickAttempt()
#define CLOTH_COLLISION_STATS_SKIP_NO_ASSET() FClothCollisionStats::AddSkippedNoAsset()
#define CLOTH_COLLISION_STATS_SKIP_NO_CLOTH_PAYLOAD() FClothCollisionStats::AddSkippedNoClothPayload()
#define CLOTH_COLLISION_STATS_SKIP_NON_CPU_SKINNING() FClothCollisionStats::AddSkippedNonCPUSkinning()
#define CLOTH_COLLISION_STATS_SKIP_NO_SKINNED_VERTICES() FClothCollisionStats::AddSkippedNoSkinnedVertices()
#define CLOTH_COLLISION_STATS_ADD_ENABLED_SECTION() FClothCollisionStats::AddEnabledSection()
#define CLOTH_COLLISION_STATS_ADD_COLLISION_ELIGIBLE_SECTION(bWorldStaticConfigured, bHasValidBounds, bHasPhysicsRuntime) \
    FClothCollisionStats::AddCollisionEligibleSection((bWorldStaticConfigured), (bHasValidBounds), (bHasPhysicsRuntime))
#define CLOTH_COLLISION_STATS_ADD_COMPONENT() FClothCollisionStats::AddComponentResult()
#define CLOTH_COLLISION_STATS_ADD_SECTION(Result, bWorldStaticEnabled) \
    FClothCollisionStats::AddSectionResult((Result), (bWorldStaticEnabled))
#else
#define CLOTH_COLLISION_STATS_RESET() ((void)0)
#define CLOTH_COLLISION_STATS_ADD_TICK_ATTEMPT() ((void)0)
#define CLOTH_COLLISION_STATS_SKIP_NO_ASSET() ((void)0)
#define CLOTH_COLLISION_STATS_SKIP_NO_CLOTH_PAYLOAD() ((void)0)
#define CLOTH_COLLISION_STATS_SKIP_NON_CPU_SKINNING() ((void)0)
#define CLOTH_COLLISION_STATS_SKIP_NO_SKINNED_VERTICES() ((void)0)
#define CLOTH_COLLISION_STATS_ADD_ENABLED_SECTION() ((void)0)
#define CLOTH_COLLISION_STATS_ADD_COLLISION_ELIGIBLE_SECTION(bWorldStaticConfigured, bHasValidBounds, bHasPhysicsRuntime) ((void)0)
#define CLOTH_COLLISION_STATS_ADD_COMPONENT() ((void)0)
#define CLOTH_COLLISION_STATS_ADD_SECTION(Result, bWorldStaticEnabled) ((void)0)
#endif
