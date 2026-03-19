#pragma once
#include "../Foundation/Math/FVector.h"
#include "../Foundation/Math/FMatrix.h"
#include "../Foundation/Containers/TArray.h"
#include "../ObjectKernel/UObject.h"
#include "../World/UPrimitiveComponent.h"
#include <memory>


struct Ray {
    FVector Origin;
    FVector Direction;

    Ray(const FVector& Origin, const FVector& Direction)
        : Origin(Origin)
        , Direction(Direction)
    {
    }

    Ray() : Origin(0.0f, 0.0f, 0.0f), Direction(0.0f, 0.0f, 1.0f) {}

    FVector GetPoint(float InT) const {
        return Origin + (Direction * InT);
    }
};

class PickingService
{
public:
    // ��ũ�� ��ǥ �� ���� Ray ����
    static Ray ScreenToRay(int32 screenX, int32 screenY,
        int32 viewportW, int32 viewportH,
        const FMatrix& viewProjInverse);

    // Ray �� ������ ������Ʈ ��ȯ (������ nullptr)
    static UPrimitiveComponent* Pick(const Ray& ray,
        const TArray<std::unique_ptr<UObject>>& objects);

    // ����: Ray-AABB ���� �׽�Ʈ
    static bool IntersectsAABB(const Ray& ray,
        const FVector& min,
        const FVector& max);

    static bool IntersectsSphere(const Ray& ray,
        const FVector& center,
        float radius,
        float& outT);

    static bool IntersectsMesh(const Ray& ray,
        const FVertexSimple* vertices,
        uint32 numVertices,
        const FMatrix& worldMatrix,
        float& outT);

    static bool IntersectsMeshIndexed(const Ray& ray, const FVertexSimple* vertices, const uint32* indices, uint32 numIndices, float& outT);

    static bool MollerTrumbore(const Ray& ray,
        const FVector& v0, const FVector& v1, const FVector& v2,
		float& outT);
};

