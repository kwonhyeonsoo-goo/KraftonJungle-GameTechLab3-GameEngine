#pragma once

#include "CoreMinimal.h"
#include <vector>
#include "Renderer/PrimitiveVertex.h"

class ENGINE_API CPrimitivePlane
{
public:
    void Generate();

    const std::vector<FPrimitiveVertex>& GetVertices() const { return Vertices; }
    const std::vector<unsigned int>& GetIndices() const { return Indices; }

private:
    std::vector<FPrimitiveVertex> Vertices;
    std::vector<unsigned int> Indices;
};
