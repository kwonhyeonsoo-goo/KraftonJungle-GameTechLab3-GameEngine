#pragma once
#include "../Foundation/Containers/FVector.h"
#include "../Foundation/Containers/FMatrix.h"

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
};