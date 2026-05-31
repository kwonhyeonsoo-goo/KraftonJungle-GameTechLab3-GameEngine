#pragma once

#include "Core/Types/CoreTypes.h"
#include "Core/Types/CollisionTypes.h"
#include "Math/Vector.h"

// Raycast 모드. 기본 gameplay 는 SnapshotImmediate(직전 publish 된 snapshot 기준, lock-free),
// 정확히 live PhysX scene 을 query 해야 하면 PhysicsThreadExact(request + fence 대기).
enum class EPhysicsQueryMode : uint8
{
    SnapshotImmediate,
    PhysicsThreadExact
};

// Physics Thread 로 넘기는 raycast 요청 — UObject pointer 없이 ID 만 담는다.
struct FPhysicsRaycastRequest
{
    uint64 RequestId  = 0;
    uint64 FrameIndex = 0;

    FVector Start       = FVector::ZeroVector;
    FVector Direction   = FVector::ZeroVector;
    float   MaxDistance = 0.0f;

    ECollisionChannel TraceChannel   = ECollisionChannel::WorldStatic;
    uint32            ObjectTypeMask = 0; // 0 이면 채널 기반, 그 외엔 ObjectType 마스크 기반

    uint32 IgnoreActorId = 0;
};

// Raycast 결과 — hit 대상을 UObject 가 아니라 ID·generation 으로 식별한다.
// Game Thread 가 binding generation 과 대조 후 FHitResult 로 resolve 한다.
struct FPhysicsRaycastResult
{
    uint64 RequestId = 0;

    bool bBlockingHit = false;

    uint32 HitActorId     = 0;
    uint32 HitComponentId = 0;
    uint32 HitGeneration  = 0;

    FVector Location     = FVector::ZeroVector;
    FVector ImpactPoint  = FVector::ZeroVector;
    FVector Normal       = FVector::ZeroVector;
    FVector ImpactNormal = FVector::ZeroVector;

    float Distance = 0.0f;
};
