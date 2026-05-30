#pragma once

#include "Object/Ptr/WeakObjectPtr.h"
#include "Physics/PhysicsTypes.h"

namespace physx
{
    class PxShape;
}

struct FShapeInstance
{
    FPhysicsShapeHandle Handle;
    FPhysicsBodyHandle  OwnerBody;

    // source component를 소유하지 않는다. callback/query 지연 처리에서 stale pointer를 막는다.
    TWeakObjectPtr<UPrimitiveComponent> SourceComponent;
    FPhysicsShapeDesc                   Desc;

    // debug draw는 이 값을 써야 실제 충돌 형상과 일치한다.
    FTransform EngineLocalTransform;
    FTransform PhysXLocalTransform;

    physx::PxShape*      Shape = nullptr;

    EPhysicsRuntimeObjectState State = EPhysicsRuntimeObjectState::Free;
};