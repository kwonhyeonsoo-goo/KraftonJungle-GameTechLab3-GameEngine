#pragma once

#include "Physics/PhysicsDebugTypes.h"
#include "Physics/PhysicsStats.h"
#include "Physics/PhysicsTypes.h"

// Physics step 이후 publish 되는 gameplay/render 적용용 불변 스냅샷.
// 이후 Physics Thread 를 도입할 때 Game Thread 는 live PhysX/BodyInstance 를 직접 읽지 않고
// 이 스냅샷만 acquire 해서 Component/Ragdoll/Vehicle visual 을 적용한다.
struct FPhysicsBodySnapshot
{
    FPhysicsBodyHandle Body;

    uint32 OwnerActorId     = 0;
    uint32 OwnerComponentId = 0;

    FName BoneName = FName::None;

    EPhysicsBodyDomain Domain = EPhysicsBodyDomain::ActorComponent;
    EPhysicsSyncMode   SyncMode = EPhysicsSyncMode::EngineToPhysics;
    EPhysicsBodyType   BodyType = EPhysicsBodyType::Static;

    FTransform PreviousTransform;
    FTransform CurrentTransform;

    FVector LinearVelocity  = FVector::ZeroVector;
    FVector AngularVelocity = FVector::ZeroVector;

    bool bIsSleeping  = false;
    bool bIsStatic    = false;
    bool bIsDynamic   = false;
    bool bIsKinematic = false;

    bool ShouldApplyToComponent() const
    {
        return Domain == EPhysicsBodyDomain::ActorComponent
            && SyncMode == EPhysicsSyncMode::PhysicsToEngine
            && OwnerComponentId != 0;
    }
};

struct FPhysicsWorldSnapshot
{
    uint64 StepIndex = 0;

    float FixedDt = 0.0f;
    float InterpolationAlpha = 0.0f;

    TArray<FPhysicsBodySnapshot> Bodies;

    // Debug renderer/stat UI 도 같은 publish boundary 를 보도록 world snapshot 에 포함한다.
    TArray<FPhysicsDebugBody>       DebugBodies;
    TArray<FPhysicsDebugConstraint> DebugConstraints;

    FPhysicsStats Stats;
};
