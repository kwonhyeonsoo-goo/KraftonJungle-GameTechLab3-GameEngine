#pragma once
#include "../Foundation/Math/FVector.h"
#include "../Foundation/Math/FMatrix.h"

struct Transform
{
    FVector Location;
    FVector Rotation;
    FVector Scale;

    Transform();
    Transform(const FVector& loc,
        const FVector& rot,
        const FVector& scale);

    FMatrix ToMatrix() const;
    FMatrix ToInverseMatrix() const;

    FVector GetLocation() const;
    FVector GetRotation() const;
    FVector GetScale() const;
};