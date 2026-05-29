#pragma once

#include "Physics/PhysicsTypes.h"

namespace physx
{
    class PxShape;
}

struct FShapeInstance
{
    FPhysicsShapeHandle Handle;
    FPhysicsBodyHandle  OwnerBody;

    UPrimitiveComponent* SourceComponent = nullptr;
    FPhysicsShapeDesc    Desc;
    physx::PxShape*      Shape = nullptr;
};