#pragma once

#include "Core/Types/CoreTypes.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"

#include <memory>

struct FSkeletalMesh;
struct FVertexPNCTT;

struct FClothWorldForceContext
{
    FVector WorldGravity = FVector(0.0f, 0.0f, -9.81f);
    FVector WorldWindVelocity = FVector::ZeroVector;
    bool bUsePreviewWindOverride = false;
};

class FSkeletalClothRuntime
{
public:
    FSkeletalClothRuntime();
    ~FSkeletalClothRuntime();

    FSkeletalClothRuntime(const FSkeletalClothRuntime&) = delete;
    FSkeletalClothRuntime& operator=(const FSkeletalClothRuntime&) = delete;

    void Reset();
    bool Tick(
        const FSkeletalMesh& Asset,
        float DeltaTime,
        TArray<FVertexPNCTT>& InOutSkinnedVertices,
        const FMatrix& ComponentWorldMatrix,
        const FClothWorldForceContext& ForceContext);
    bool IsActive() const;

private:
    struct FImpl;
    std::unique_ptr<FImpl> Impl;
};
