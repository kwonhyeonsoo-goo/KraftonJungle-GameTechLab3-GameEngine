#pragma once
#include "../Foundation/Math/FVector.h"
#include "../Foundation/Math/FQuat.h"
#include "../Foundation/Math/FMatrix.h"

struct Transform
{
    FVector Location;
    FQuat Rotation;
    FVector Scale;

    Transform();
    Transform(const FVector& loc,
        const FQuat& rot,
        const FVector& scale);

    static Transform FromEulerDegrees(const FVector& loc,
        const FVector& rotDeg,
        const FVector& scale);

    FMatrix ToMatrix() const;
    FMatrix ToInverseMatrix() const;

    FVector GetLocation() const;
    FQuat GetRotation() const;
    FVector GetScale() const;

    FVector GetEulerDegrees() const;
    FVector GetEulerDegreesNearest(const FVector& referenceDeg) const;
    void SetEulerDegrees(const FVector& eulerDeg);
};
