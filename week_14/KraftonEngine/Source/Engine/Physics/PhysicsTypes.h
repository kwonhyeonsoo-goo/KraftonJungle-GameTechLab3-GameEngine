#pragma once

#include "Core/Types/CollisionTypes.h"
#include "Core/Types/CoreTypes.h"
#include "Math/Transform.h"
#include "Math/Vector.h"
#include "Object/FName.h"
#include "Physics/PhysicsBodyHandle.h"

class AActor;
class UPrimitiveComponent;

enum class EPhysicsBodyType : uint8
{
    Static,
    Dynamic,
    Kinematic
};

enum class EPhysicsSyncMode : uint8
{
    EngineToPhysics,
    PhysicsToEngine,
    KinematicTarget,
    Manual
};

enum class EPhysicsShapeType : uint8
{
    Box,
    Sphere,
    Capsule,
    Convex,
    TriangleMesh
};

enum class EPhysicsTeleportMode : uint8
{
    None,
    TeleportPhysics
};

enum class EConstraintMotion : uint8
{
    Free,
    Limited,
    Locked
};

struct FPhysicsFilterData
{
    uint32 ObjectType  = 0;
    uint32 BlockMask   = 0;
    uint32 OverlapMask = 0;

    uint32 IgnoreGroup = 0;
};

struct FPhysicsShapeDesc
{
    EPhysicsShapeType Type = EPhysicsShapeType::Box;

    FTransform LocalTransform;

    FVector BoxHalfExtent = FVector(50.0f, 50.0f, 50.0f);

    float SphereRadius = 50.0f;

    float CapsuleRadius     = 50.0f;
    float CapsuleHalfHeight = 100.0f;

    ECollisionEnabled CollisionEnabled = ECollisionEnabled::QueryAndPhysics;

    FPhysicsFilterData FilterData;

    bool bIsTrigger = false;
};

struct FBodyCreationDesc
{
    AActor*              OwnerActor     = nullptr;
    UPrimitiveComponent* OwnerComponent = nullptr;

    // Skeletal/Ragdoll body면 BoneName을 채웁니다.
    FName BoneName = FName::None;

    EPhysicsBodyType BodyType = EPhysicsBodyType::Static;
    EPhysicsSyncMode SyncMode = EPhysicsSyncMode::EngineToPhysics;

    FTransform WorldTransform;

    TArray<FPhysicsShapeDesc> Shapes;

    float   Mass                    = 1.0f;
    FVector CenterOfMassLocalOffset = FVector::ZeroVector;

    float LinearDamping  = 0.0f;
    float AngularDamping = 0.0f;

    bool bEnableCCD             = false;
    bool bGenerateHitEvents     = false;
    bool bGenerateOverlapEvents = false;
};

struct FConstraintLimitDesc
{
    EConstraintMotion LinearX = EConstraintMotion::Locked;
    EConstraintMotion LinearY = EConstraintMotion::Locked;
    EConstraintMotion LinearZ = EConstraintMotion::Locked;

    EConstraintMotion Twist  = EConstraintMotion::Limited;
    EConstraintMotion Swing1 = EConstraintMotion::Limited;
    EConstraintMotion Swing2 = EConstraintMotion::Limited;

    float TwistLimitMinDegrees = -45.0f;
    float TwistLimitMaxDegrees = 45.0f;

    float Swing1LimitDegrees = 30.0f;
    float Swing2LimitDegrees = 30.0f;

    bool bEnableProjection = true;
};

struct FConstraintCreationDesc
{
    FTransform ParentLocalFrame;
    FTransform ChildLocalFrame;

    FConstraintLimitDesc Limits;

    bool bDisableCollisionBetweenBodies = true;
};