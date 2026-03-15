#include "PickingService.h"

#include "../Foundation/Core/CoreTypes.h"
#include "../World/UPrimitiveComponent.h"
#include "../Foundation/Math/FVector.h"
#include "../Foundation/Math/FVector4.h"
#include "../Foundation/Math/FMatrix.h"
#include "../Foundation/Math/FMatrix4.h"
#include <iostream>

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
    FVector4 worldFar = ndcFar * viewProjInverse;

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
        UPrimitiveComponent* Prim = static_cast<UPrimitiveComponent*>(object);

        if (!Prim || !Prim->bVisible) continue;

        FMatrix WorldMat = Prim->GetWorldMatrix();
        FMatrix InvWorld = WorldMat.Inversed();

        Ray LocalRay;
        LocalRay.Origin = InvWorld.TransformPosition(ray.Origin);
        LocalRay.Direction = InvWorld.TransformVector(ray.Direction);

        float t = FLT_MAX;
        bool hit = false;
        switch (Prim->Shape)
        {
        case EPrimitiveShape::Sphere:
            hit = IntersectsSphere(LocalRay, FVector(0.0f, 0.0f, 0.0f), 1, t);
            break;

        case EPrimitiveShape::Plane:
            hit = IntersectsPlane(LocalRay, t);
            break;

        case EPrimitiveShape::Cube:
        case EPrimitiveShape::Triangle:
        default:
            hit = IntersectsAABB(LocalRay, Prim->GetBoundMin(), Prim->GetBoundMax(), t);
            break;
        }

        if (!hit || t <= 0.0f) continue;

        FVector localHit = LocalRay.Origin + LocalRay.Direction * t;
        FVector worldHit = WorldMat.TransformPosition(localHit);
        float   worldDist = (worldHit - ray.Origin).Length();

        if (worldDist < ObjTime) {
            ObjTime = worldDist;
            CloseObj = Prim;
        }
    }

    if (CloseObj)
    {
        // 1. 도형 타입을 문자열로 변환
        std::string ShapeName;
        switch (CloseObj->Shape)
        {
        case EPrimitiveShape::Sphere:   ShapeName = "Sphere"; break;
        case EPrimitiveShape::Plane:    ShapeName = "Plane"; break;
        case EPrimitiveShape::Cube:     ShapeName = "Cube"; break;
        case EPrimitiveShape::Triangle: ShapeName = "Triangle"; break;
        default:                        ShapeName = "Unknown"; break;
        }

        // 2. 상세 정보 출력
        std::cout << "========================================" << std::endl;
        std::cout << "[Pick Success]" << std::endl;
        std::cout << " - Target Shape : " << ShapeName << std::endl;
        std::cout << " - Distance     : " << ObjTime << " units" << std::endl;

        // 물체의 현재 위치 출력
        FVector Loc = CloseObj->GetTransform().GetLocation();
        std::cout << " - Actor Loc    : (" << Loc.x << ", " << Loc.y << ", " << Loc.z << ")" << std::endl;
        std::cout << "========================================" << std::endl;
    }
    else
    {
        // 아무것도 클릭되지 않았을 때
        // std::cout << "[Pick Failed] No object hit." << std::endl;
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

bool PickingService::IntersectsSphere(const Ray& ray,
    const FVector& center,
    float radius,
    float& outT)
{
    FVector L = center - ray.Origin;

    float tc = L.Dot(ray.Direction);
    if (tc < 0.0f) return false;

    float d2 = L.Dot(L) - tc * tc;
    float radius2 = radius * radius;

    if (d2 > radius2) return false;

    float t1c = sqrtf(radius2 - d2);
    float t1 = tc - t1c;
    float t2 = tc + t1c;

    outT = (t1 > 0.0f) ? t1 : t2;
    return outT > 0.0f;
}

bool PickingService::IntersectsPlane(const Ray& ray, float& outT)
{
    // 로컬 공간: 법선 = Z축, 평면 위치 = Z=0
    float denom = ray.Direction.z;

    // Ray가 평면에 평행
    if (fabs(denom) < 1e-6f) return false;

    float t = -ray.Origin.z / denom;

    // 평면이 카메라 뒤에 있음
    if (t < 0.0f) return false;

    // 교차점이 평면 범위 안인지 확인 [-0.5, 0.5]
    FVector hit = ray.Origin + ray.Direction * t;
    if (hit.x < -0.5f || hit.x > 0.5f) return false;
    if (hit.y < -0.5f || hit.y > 0.5f) return false;

    outT = t;
    return true;
}