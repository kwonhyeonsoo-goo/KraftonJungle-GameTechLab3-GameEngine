#include "PickingService.h"
#include <memory>

#include "../Foundation/Core/CoreTypes.h"
#include "../World/UPrimitiveComponent.h"
#include "../Foundation/Math/FVector.h"
#include "../Foundation/Math/FVector4.h"
#include "../Foundation/Math/FMatrix.h"
#include "../Foundation/Math/FMatrix4.h"
#include <iostream>

/// <summary>
/// ���콺 ��ǥ�� ����Ʈ ���� ��ǥ�� ��ȯ�ؼ� �����������
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

    //std::cout << screenX << " " << screenY << " " << viewportW << " " << viewportH << std::endl;


    FVector4 ndcNear = FVector4(ndcX, ndcY, 0.0f, 1.0f); // Z=0 Near ���
    FVector4 ndcFar = FVector4(ndcX, ndcY, 1.0f, 1.0f); // Z=1 Far ���

    FVector4 worldNear = ndcNear * viewProjInverse;
    FVector4 worldFar = ndcFar * viewProjInverse;

    if(worldNear.w != 0.0f) worldNear = worldNear * (1.0f / worldNear.w);
    if (worldFar.w != 0.0f) worldFar = worldFar * (1.0f / worldFar.w);


    FVector origin = worldNear.ToVector();
    FVector direction = (worldFar.ToVector() - origin).Normalize();

    //std::cout << origin.x << " " << origin.y << " " << origin.z << std::endl;

   return Ray(origin, direction);
}

UPrimitiveComponent* PickingService::Pick(const Ray& ray, const TArray<std::unique_ptr<UObject>>& objects)
{
    UPrimitiveComponent* CloseObj = nullptr;
    float ObjTime = FLT_MAX;

    for (const auto& obj_uptr : objects) {
        UObject* object = obj_uptr.get();
        if (!object->IsA<UPrimitiveComponent>()) continue;
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
    return CloseObj; // ������ nullptr
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
            // Ray�� �ش� �࿡ ���� �� ���� ���̸� ���� ����
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
    // ���� ����: ���� = Z��, ��� ��ġ = Z=0
    float denom = ray.Direction.z;

    // Ray�� ��鿡 ����
    if (fabs(denom) < 1e-6f) return false;

    float t = -ray.Origin.z / denom;

    // ����� ī�޶� �ڿ� ����
    if (t < 0.0f) return false;

    // �������� ��� ���� ������ Ȯ�� [-0.5, 0.5]
    FVector hit = ray.Origin + ray.Direction * t;
    
    if (hit.x < -0.5f || hit.x > 0.5f) return false;
    if (hit.y < -0.5f || hit.y > 0.5f) return false;

    outT = t;
    return true;
}