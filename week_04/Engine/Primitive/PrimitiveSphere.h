#pragma once

#include "CoreMinimal.h"
#include <vector>
#include "Renderer/PrimitiveVertex.h"

class ENGINE_API CPrimitiveSphere
{
public:
    void Generate(int Segments = 16, int Rings = 16);

    const std::vector<FPrimitiveVertex>& GetVertices() const { return Vertices; }
    const std::vector<unsigned int>& GetIndices() const { return Indices; }

private:
    std::vector<FPrimitiveVertex> Vertices;
    std::vector<unsigned int> Indices;
};
