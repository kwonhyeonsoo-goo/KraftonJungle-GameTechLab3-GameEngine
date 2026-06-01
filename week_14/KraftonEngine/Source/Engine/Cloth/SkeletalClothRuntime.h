#pragma once

#include "Core/Types/CoreTypes.h"

#include <memory>

struct FSkeletalMesh;
struct FVertexPNCTT;

class FSkeletalClothRuntime
{
public:
    FSkeletalClothRuntime();
    ~FSkeletalClothRuntime();

    FSkeletalClothRuntime(const FSkeletalClothRuntime&) = delete;
    FSkeletalClothRuntime& operator=(const FSkeletalClothRuntime&) = delete;

    void Reset();
    bool Tick(const FSkeletalMesh& Asset, float DeltaTime, TArray<FVertexPNCTT>& InOutSkinnedVertices);
    bool IsActive() const;

private:
    struct FImpl;
    std::unique_ptr<FImpl> Impl;
};
