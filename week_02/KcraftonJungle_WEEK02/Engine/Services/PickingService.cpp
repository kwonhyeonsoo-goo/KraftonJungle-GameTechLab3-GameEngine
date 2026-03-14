#include "PickingService.h"

#include "../Foundation/Core/CoreTypes.h"
#include "../World/UPrimitiveComponent.h"
#include "../Foundation/Math/FVector.h"
#include "../Foundation/Math/FVector4.h"
#include "../Foundation/Math/FMatrix.h"
#include "../Foundation/Math/FMatrix4.h"

/// <summary>
/// 마우스 좌표를 뷰포트 로컬 좌표로 변환해서 전달해줘야함
/// </summary>
/// <param name="screenX"></param>
/// <param name="screenY"></param>
/// <param name="viewportW"></param>
/// <param name="viewportH"></param>
/// <param name="viewProjInverse"></param>
/// <returns></returns>
Ray PickingService::ScreenToRay(int32 screenX, int32 screenY, int32 viewportW, int32 viewportH, const FMatrix& viewProjInverse)
{
    float ndcX = (2.0f * screenX / viewportW) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY / viewportH);

    FVector4 ndcNear = FVector4(ndcX, ndcY, 0.0f, 1.0f); // Z=0 Near 평면
    FVector4 ndcFar = FVector4(ndcX, ndcY, 1.0f, 1.0f); // Z=1 Far 평면

    FVector4 worldNear = ndcNear * viewProjInverse;
    FVector4 worldFar = ndcFar * viewProjInverse;;

    if(worldNear.w != 0.0f) worldNear = worldNear * (1.0f / worldNear.w);
    if (worldFar.w != 0.0f) worldFar = worldFar * (1.0f / worldFar.w);

    FVector origin = worldNear.ToVector();
    FVector direction = (worldFar.ToVector() - origin).Normalize();

	return Ray(origin, direction);
}

UPrimitiveComponent* PickingService::Pick(const Ray& ray, const TArray<UObject*>& objects)
{
    UPrimitiveComponent* CloseObj = nullptr;
    float ObjTime = FLT_MAX;

    for (auto* object : objects) {//TODO: 여기도 RTTI로 바꿔야함
        UPrimitiveComponent* Prim = dynamic_cast<UPrimitiveComponent*>(object);

        if (!Prim || !Prim->bVisible) continue;

        // AABB를 World 공간으로 변환
        FMatrix world = Prim->GetWorldMatrix();
        FVector BoundMin = Prim->GetBoundMin();
        FVector BoundMax = Prim->GetBoundMax();

        // BoundsMin/Max를 World 공간 8개 꼭짓점으로 변환 후 재계산
        FVector corners[8] = {
            FVector(BoundMin.x, BoundMin.y, BoundMin.z),
            FVector(BoundMax.x, BoundMin.y, BoundMin.z),
            FVector(BoundMin.x, BoundMax.y, BoundMin.z),
            FVector(BoundMax.x, BoundMax.y, BoundMin.z),
            FVector(BoundMin.x, BoundMin.y, BoundMax.z),
            FVector(BoundMax.x, BoundMin.y, BoundMax.z),
            FVector(BoundMin.x, BoundMax.y, BoundMax.z),
            FVector(BoundMax.x, BoundMax.y, BoundMax.z),
        };

        FVector WorldMin = FVector(FLT_MAX, FLT_MAX, FLT_MAX);
        FVector WorldMax = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (FVector& c : corners) {
            FVector4 wc = FVector4(c.x, c.y, c.z, 1.0f) * world;
            WorldMin.x = std::min(WorldMin.x, wc.x);
            WorldMin.y = std::min(WorldMin.y, wc.y);
            WorldMin.z = std::min(WorldMin.z, wc.z);
            WorldMax.x = std::max(WorldMax.x, wc.x);
            WorldMax.y = std::max(WorldMax.y, wc.y);
            WorldMax.z = std::max(WorldMax.z, wc.z);
        }

        float t = FLT_MAX;
        if (IntersectsAABB(ray, WorldMin, WorldMax, t)) {
            if (t < ObjTime && t > 0) {
                ObjTime = t;
                CloseObj = Prim;
            }
        }
    }

    return CloseObj; // 없으면 nullptr
}

bool PickingService::IntersectsAABB(const Ray& ray, const FVector& min, const FVector& max, float& outT)
{
    float tMin = 0.0f;
    float tMax = FLT_MAX;

    for (int i = 0; i < 3; i++) {
        float origin = (&ray.Origin.x)[i];
        float dir = (&ray.Direction.x)[i];
        float bMin = (&min.x)[i];
        float bMax = (&max.x)[i];

        if (fabs(dir) < 1e-6f) {
            // Ray가 해당 축에 평행 → 슬랩 밖이면 교차 없음
            if (origin < bMin || origin > bMax)
                return false;
        }
        else {
            float t1 = (bMin - origin) / dir;
            float t2 = (bMax - origin) / dir;

            if (t1 > t2) std::swap(t1, t2);

            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            if (tMin > tMax)
                return false;
        }
    }

    outT = tMin;
    return true;
}
