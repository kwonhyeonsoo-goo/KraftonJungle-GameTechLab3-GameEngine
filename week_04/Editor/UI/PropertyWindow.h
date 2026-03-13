#pragma once

#include "Math/Vector.h"

class CPropertyWindow
{
public:
    void Render();

    void SetTarget(const FVector& Location, const FVector& Rotation, const FVector& Scale);
    bool IsModified() const { return bModified; }

    FVector GetLocation() const { return EditLocation; }
    FVector GetRotation() const { return EditRotation; }
    FVector GetScale() const { return EditScale; }

private:
    FVector EditLocation;
    FVector EditRotation;
    FVector EditScale = { 1.0f, 1.0f, 1.0f };
    bool bModified = false;
};
