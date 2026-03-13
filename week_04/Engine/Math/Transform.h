#pragma once

#include "EngineAPI.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

struct ENGINE_API FTransform
{
    FVector Location;
    FVector Rotation;
    FVector Scale = { 1.0f, 1.0f, 1.0f };

    FTransform() = default;
    FTransform(const FVector& InLocation, const FVector& InRotation, const FVector& InScale)
        : Location(InLocation), Rotation(InRotation), Scale(InScale) {}

    FMatrix ToMatrix() const;
};
