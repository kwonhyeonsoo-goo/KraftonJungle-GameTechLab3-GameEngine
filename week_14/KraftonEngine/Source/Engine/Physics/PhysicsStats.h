#pragma once

#include "Core/Types/CoreTypes.h"

struct FPhysicsStats
{
    int32 NumBodies          = 0;
    int32 NumStaticBodies    = 0;
    int32 NumDynamicBodies   = 0;
    int32 NumKinematicBodies = 0;

    int32 NumShapes      = 0;
    int32 NumConstraints = 0;

    int32 NumActiveBodies   = 0;
    int32 NumSleepingBodies = 0;

    int32 NumContactPairs = 0;
    int32 NumTriggerPairs = 0;

    int32 NumSubsteps = 0;

    float PrePhysicsMs    = 0.0f;
    float SimulateMs      = 0.0f;
    float FetchResultsMs  = 0.0f;
    float PostPhysicsMs   = 0.0f;
    float DispatchEventMs = 0.0f;
};