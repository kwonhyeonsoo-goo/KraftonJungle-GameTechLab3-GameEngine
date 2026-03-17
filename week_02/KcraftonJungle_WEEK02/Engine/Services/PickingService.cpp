#include "PickingService.h"
#include <memory>

#include "../Foundation/Core/CoreTypes.h"
#include "../World/UPrimitiveComponent.h"
#include "../Foundation/Math/FVector.h"
#include "../Foundation/Math/FVector4.h"
#include "../Foundation/Math/FMatrix.h"
#include "../Foundation/Math/FMatrix4.h"
#include <iostream>
#include "../Mesh/FVertexSimple.h"

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
    static const uint32 PlaneIndices[] = { 0, 2, 1, 0, 3, 2 };
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

        if (!IntersectsAABB(LocalRay, Prim->GetBoundMin(), Prim->GetBoundMax())) continue;

        switch (Prim->Shape)
        {
        case EPrimitiveShape::Sphere:
            hit = IntersectsSphere(LocalRay, FVector(0.0f, 0.0f, 0.0f), 1.0f, t);
            break;

        case EPrimitiveShape::Plane:
			hit = IntersectsMeshIndexed(LocalRay, Prim->Vertices, PlaneIndices, 6, t);
            break;

        case EPrimitiveShape::Cube:
        case EPrimitiveShape::Triangle:
        default:
			hit = IntersectsMesh(LocalRay, Prim->Vertices, Prim->NumVertices, WorldMat, t);
            //hit = IntersectsAABB(LocalRay, Prim->GetBoundMin(), Prim->GetBoundMax(), t);
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

bool PickingService::IntersectsAABB(const Ray& ray, const FVector& min, const FVector& max)
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

bool PickingService::IntersectsMesh(const Ray& ray, const FVertexSimple* vertices, uint32 numVertices, const FMatrix& worldMatrix, float& outT)
{
    float minT = FLT_MAX;
	bool hit = false;

    if (vertices == nullptr || numVertices < 3)
    {
        return false;
    }

    for (uint32 i = 0; i + 2 < numVertices; i += 3) {
        FVector v0 = FVector(vertices[i].x, vertices[i].y, vertices[i].z);
        FVector v1 = FVector(vertices[i + 1].x, vertices[i + 1].y, vertices[i + 1].z);
        FVector v2 = FVector(vertices[i + 2].x, vertices[i + 2].y, vertices[i + 2].z);

        float t = 0.f;
        if (MollerTrumbore(ray, v0, v1, v2, t)) {
            if (t > 0.f && t < minT) {
                minT = t;
                hit = true;
            }
        }
    }

	outT = minT;
    return hit;
}

bool PickingService::IntersectsMeshIndexed(
    const Ray& ray,
    const FVertexSimple* vertices,
    const uint32* indices,
    uint32 numIndices,
    float& outT)
{
    float minT = FLT_MAX;
    bool hit = false;

    if (vertices == nullptr || indices == nullptr || numIndices < 3)
    {
        return false;
    }

    // 3개씩 묶어서 삼각형으로
    for (uint32 i = 0; i + 2 < numIndices; i += 3) {
        // 인덱스로 정점 참조
        uint32 idx0 = indices[i];
        uint32 idx1 = indices[i + 1];
        uint32 idx2 = indices[i + 2];

        FVector v0 = FVector(vertices[idx0].x, vertices[idx0].y, vertices[idx0].z);
        FVector v1 = FVector(vertices[idx1].x, vertices[idx1].y, vertices[idx1].z);
        FVector v2 = FVector(vertices[idx2].x, vertices[idx2].y, vertices[idx2].z);

        float t = 0.f;
        if (MollerTrumbore(ray, v0, v1, v2, t)) {
            if (t > 0.f && t < minT) {
                minT = t;
                hit = true;
            }
        }
    }

    outT = minT;
    return hit;
}

bool PickingService::MollerTrumbore(const Ray& ray, const FVector& a, const FVector& b, const FVector& c, float& outT)
{
    FVector Q = ray.Direction.Cross(b - a); // 한 번만 계산
    FVector P = (ray.Origin - a).Cross(c - a); // 한 번만 계산
    float denom = Q.Dot(c - a);

    if (fabs(denom) < 1e-8f) return false;

    float invDenom = 1.0f / denom;
    float t = P.Dot(b - a) * invDenom;
    float u = (ray.Origin - a).Dot(Q) * invDenom;
    float v = ray.Direction.Dot(P) * invDenom;


    if (t < 0.f || u < 0.f|| v < 0.f) return false; // 삼각형 밖
    if (u + v > 1.f)       return false; // 삼각형 밖

    outT = t;
    return true;
}
