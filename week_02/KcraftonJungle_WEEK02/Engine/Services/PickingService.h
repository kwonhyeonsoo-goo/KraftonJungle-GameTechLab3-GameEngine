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
        const FVector& max,
        float& outT);

    static bool IntersectsSphere(const Ray& ray,
        const FVector& center,
        float radius,
        float& outT);

    static bool IntersectsPlane(const Ray& ray, 
        float& outT);
};

