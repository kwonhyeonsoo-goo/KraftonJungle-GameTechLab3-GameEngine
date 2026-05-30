#include "Physics/PhysXBodyBuilder.h"
#include "Physics/PhysXConversion.h"

#include <algorithm>
#include <PxPhysicsAPI.h>

using namespace physx;

namespace
{
    PxFilterData ToPxFilterData(const FPhysicsFilterData& In)
    {
        PxFilterData Out;
        Out.word0 = In.ObjectType;
        Out.word1 = In.BlockMask;
        Out.word2 = In.OverlapMask;
        Out.word3 = In.IgnoreGroup;
        return Out;
    }
}

PxRigidActor* FPhysXBodyBuilder::CreateRigidActor(PxPhysics* Physics, const FBodyCreationDesc& Desc)
{
    if (!Physics)
    {
        return nullptr;
    }

    const PxTransform Pose = ToPxTransform(Desc.WorldTransform);

    if (Desc.BodyType == EPhysicsBodyType::Static)
    {
        return Physics->createRigidStatic(Pose);
    }

    PxRigidDynamic* Dynamic = Physics->createRigidDynamic(Pose);
    if (!Dynamic)
    {
        return nullptr;
    }

    if (Desc.BodyType == EPhysicsBodyType::Kinematic)
    {
        Dynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    }

    ApplyBodyProperties(Dynamic, Desc);
    return Dynamic;
}

PxShape* FPhysXBodyBuilder::CreateShape(PxPhysics* Physics, PxMaterial* DefaultMaterial, const FPhysicsShapeDesc& Desc)
{
    if (!Physics || !DefaultMaterial)
    {
        return nullptr;
    }

    PxGeometryHolder Geometry;
    bool bHasGeometry = false;
    PxQuat ShapeAxisRotation(PxIdentity);

    if (Desc.Type == EPhysicsShapeType::Box)
    {
        Geometry = PxBoxGeometry(
            Desc.BoxHalfExtent.X,
            Desc.BoxHalfExtent.Y,
            Desc.BoxHalfExtent.Z
        );
        bHasGeometry = true;
    }
    else if (Desc.Type == EPhysicsShapeType::Sphere)
    {
        Geometry = PxSphereGeometry(Desc.SphereRadius);
        bHasGeometry = true;
    }
    else if (Desc.Type == EPhysicsShapeType::Capsule)
    {
        const float PhysXHalfHeight = std::max(0.0f, Desc.CapsuleHalfHeight - Desc.CapsuleRadius);
        Geometry = PxCapsuleGeometry(Desc.CapsuleRadius, PhysXHalfHeight);

        // PxCapsuleGeometry의 기본 축은 X축입니다. 기존 엔진의 Z-up 캡슐 표현과 맞추기 위해
        // 기존 PhysXPhysicsScene.cpp에서 쓰던 보정 회전을 동일하게 유지합니다.
        ShapeAxisRotation = PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f));
        bHasGeometry = true;
    }

    if (!bHasGeometry)
    {
        return nullptr;
    }

    PxShape* Shape = Physics->createShape(Geometry.any(), *DefaultMaterial, true);
    if (!Shape)
    {
        return nullptr;
    }

    PxTransform LocalPose = ToPxTransform(Desc.LocalTransform);
    LocalPose.q = LocalPose.q * ShapeAxisRotation;
    Shape->setLocalPose(LocalPose);

    ApplyShapeProperties(Shape, Desc);
    return Shape;
}

void FPhysXBodyBuilder::ApplyShapeProperties(PxShape* Shape, const FPhysicsShapeDesc& Desc)
{
    if (!Shape)
    {
        return;
    }

    const PxFilterData FilterData = ToPxFilterData(Desc.FilterData);
    Shape->setSimulationFilterData(FilterData);
    Shape->setQueryFilterData(FilterData);

    if (Desc.CollisionEnabled == ECollisionEnabled::NoCollision)
    {
        Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
        Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
        Shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
        return;
    }

    const bool bQueryEnabled =
        Desc.CollisionEnabled == ECollisionEnabled::QueryOnly ||
        Desc.CollisionEnabled == ECollisionEnabled::QueryAndPhysics;

    const bool bOverlapOnlyNeedsSimulation =
            Desc.CollisionEnabled == ECollisionEnabled::QueryOnly &&
            Desc.FilterData.OverlapMask != 0 &&
            !Desc.bIsTrigger;

    Shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, bQueryEnabled);

    if (Desc.bIsTrigger)
    {
        Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
        Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
    }
    else
    {
        Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
        Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE,
            Desc.CollisionEnabled == ECollisionEnabled::PhysicsOnly ||
            Desc.CollisionEnabled == ECollisionEnabled::QueryAndPhysics ||
            bOverlapOnlyNeedsSimulation
        );
    }
}

void FPhysXBodyBuilder::ApplyBodyProperties(PxRigidActor* Actor, const FBodyCreationDesc& Desc)
{
    if (!Actor)
    {
        return;
    }

    if (PxRigidDynamic* Dynamic = Actor->is<PxRigidDynamic>())
    {
        Dynamic->setLinearDamping(Desc.LinearDamping);
        Dynamic->setAngularDamping(Desc.AngularDamping);
        Dynamic->setMaxAngularVelocity(Desc.MaxAngularVelocity);
        Dynamic->setSolverIterationCounts(
            static_cast<PxU32>((std::max)(1, Desc.PositionSolverIterationCount)),
            static_cast<PxU32>((std::max)(1, Desc.VelocitySolverIterationCount))
        );
        Dynamic->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, Desc.bEnableCCD);
        Dynamic->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !Desc.bEnableGravity);

        Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X, Desc.bLockLinearX);
        Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, Desc.bLockLinearY);
        Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, Desc.bLockLinearZ);
        Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, Desc.bLockAngularX);
        Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, Desc.bLockAngularY);
        Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, Desc.bLockAngularZ);
    }
}

void FPhysXBodyBuilder::UpdateMassAndInertia(PxRigidActor* Actor, const FBodyCreationDesc& Desc)
{
    if (!Actor)
    {
        return;
    }

    PxRigidDynamic* Dynamic = Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return;
    }

    const float Mass = Desc.Mass > 0.0f ? Desc.Mass : 1.0f;
    PxVec3 LocalCOM = ToPxVec3(Desc.CenterOfMassLocalOffset);

    PxRigidBodyExt::setMassAndUpdateInertia(*Dynamic, Mass, &LocalCOM);
    Dynamic->setCMassLocalPose(PxTransform(LocalCOM));
}
