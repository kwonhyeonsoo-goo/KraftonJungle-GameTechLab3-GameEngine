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

// Runtime object(Body/Shape/Constraint) lifecycle 상태. generation 과 함께 stale access
// 와 simulate/callback 중 release 를 막는다.
enum class EPhysicsRuntimeObjectState : uint8
{
    Free,
    PendingCreate,
    Alive,
    PendingDestroy,
    Destroyed
};

// Body 의 도메인 — Debug/Stat 분리, compound vs ragdoll 정책 분기에 사용.
enum class EPhysicsBodyDomain : uint8
{
    ActorComponent,
    Ragdoll,
    Vehicle,
    ClothCollision,
    EditorPreview
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

// Runtime 에서 보관하는 body 의 mutable 속성 묶음. SetMass/SetCOM/SetLock 등이 서로의 값을
// 덮어쓰지 않도록 단일 소스로 들고, RebuildBody 후 ApplyBodyRuntimeProperties 로 재적용한다.
struct FBodyRuntimeProperties
{
    float   Mass                    = 1.0f;
    FVector CenterOfMassLocalOffset = FVector::ZeroVector;

    float LinearDamping  = 0.0f;
    float AngularDamping = 0.0f;

    float MaxAngularVelocity = 100.0f;

    int32 PositionSolverIterationCount = 8;
    int32 VelocitySolverIterationCount = 2;

    bool bEnableCCD     = false;
    bool bEnableGravity = true;

    bool bLockLinearX  = false;
    bool bLockLinearY  = false;
    bool bLockLinearZ  = false;
    bool bLockAngularX = false;
    bool bLockAngularY = false;
    bool bLockAngularZ = false;
};

struct FBodyCreationDesc
{
    AActor*              OwnerActor     = nullptr;
    UPrimitiveComponent* OwnerComponent = nullptr;

    EPhysicsBodyDomain Domain = EPhysicsBodyDomain::ActorComponent;

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

    bool bLockLinearX = false;
    bool bLockLinearY = false;
    bool bLockLinearZ = false;
    bool bLockAngularX = false;
    bool bLockAngularY = false;
    bool bLockAngularZ = false;

    // Vehicle/Ragdoll 안정성 파라미터 — dynamic body 생성 시 적용.
    float MaxAngularVelocity           = 100.0f;
    int32 PositionSolverIterationCount = 8;
    int32 VelocitySolverIterationCount = 2;
    bool  bEnableGravity               = true;

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