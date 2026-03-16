#include "Picker.h"
#include "Object/Scene/Scene.h"
#include "Object/Actor/Actor.h"
#include "Camera/Camera.h"
#include "Component/PrimitiveComponent.h"
#include "Primitive/PrimitiveBase.h"
#include "Renderer/PrimitiveVertex.h"
#include <cmath>
#include <limits>

FRay CPicker::ScreenToRay(int32 ScreenX, int32 ScreenY, int32 ScreenWidth, int32 ScreenHeight,
                           const FMatrix& ViewMatrix, const FMatrix& ProjMatrix) const
{
    // Screen → NDC (-1 ~ +1)
    float NdcX = (2.0f * ScreenX / ScreenWidth) - 1.0f;
    float NdcY = 1.0f - (2.0f * ScreenY / ScreenHeight);

    // NDC → View Space (Projection 역변환)
    const float ViewForward = 1.0f;
    const float ViewRight = NdcX / ProjMatrix.M[1][0];
    const float ViewUp = NdcY / ProjMatrix.M[2][1];

    // View → World (View 행렬의 역행렬)
    FMatrix ViewInv = ViewMatrix.GetInverse();

    // 레이 방향 (View Space의 방향 벡터를 World로 변환)
    FVector RayDirWorld;
    RayDirWorld.X = ViewForward * ViewInv.M[0][0] + ViewRight * ViewInv.M[1][0] + ViewUp * ViewInv.M[2][0];
    RayDirWorld.Y = ViewForward * ViewInv.M[0][1] + ViewRight * ViewInv.M[1][1] + ViewUp * ViewInv.M[2][1];
    RayDirWorld.Z = ViewForward * ViewInv.M[0][2] + ViewRight * ViewInv.M[1][2] + ViewUp * ViewInv.M[2][2];
    RayDirWorld = RayDirWorld.GetSafeNormal();

    // 레이 원점 = 카메라 위치 (ViewInverse의 Translation 부분)
    FVector RayOrigin;
    RayOrigin.X = ViewInv.M[3][0];
    RayOrigin.Y = ViewInv.M[3][1];
    RayOrigin.Z = ViewInv.M[3][2];

    return { RayOrigin, RayDirWorld };
}

bool CPicker::RayTriangleIntersect(const FRay& Ray,
                                    const FVector& V0, const FVector& V1, const FVector& V2,
                                    float& OutDistance) const
{
    const float EPSILON = 1e-6f;

    FVector Edge1 = V1 - V0;
    FVector Edge2 = V2 - V0;

    FVector H = FVector::CrossProduct(Ray.Direction, Edge2);
    float A = FVector::DotProduct(Edge1, H);

    if (A > -EPSILON && A < EPSILON)
        return false; // 레이가 삼각형과 평행

    float F = 1.0f / A;
    FVector S = Ray.Origin - V0;
    float U = F * FVector::DotProduct(S, H);

    if (U < 0.0f || U > 1.0f)
        return false;

    FVector Q = FVector::CrossProduct(S, Edge1);
    float V = F * FVector::DotProduct(Ray.Direction, Q);

    if (V < 0.0f || U + V > 1.0f)
        return false;

    float T = F * FVector::DotProduct(Edge2, Q);

    if (T > EPSILON)
    {
        OutDistance = T;
        return true;
    }

    return false;
}

AActor* CPicker::PickActor(UScene* Scene, int32 ScreenX, int32 ScreenY,
                            int32 ScreenWidth, int32 ScreenHeight) const
{
    if (!Scene || !Scene->GetCamera())
        return nullptr;

    CCamera* Camera = Scene->GetCamera();
    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjMatrix = Camera->GetProjectionMatrix();

    FRay Ray = ScreenToRay(ScreenX, ScreenY, ScreenWidth, ScreenHeight, ViewMatrix, ProjMatrix);

    AActor* ClosestActor = nullptr;
    float ClosestDist = (std::numeric_limits<float>::max)();

    for (AActor* Actor : Scene->GetActors())
    {
        if (!Actor || Actor->IsPendingDestroy())
            continue;

        for (UActorComponent* Comp : Actor->GetComponents())
        {
            UPrimitiveComponent* PrimComp = dynamic_cast<UPrimitiveComponent*>(Comp);
            if (!PrimComp || !PrimComp->GetPrimitive())
                continue;

            FMeshData* Mesh = PrimComp->GetPrimitive()->GetMeshData();
            if (!Mesh)
                continue;

            FMatrix W = PrimComp->GetWorldTransform();

            // 각 삼각형에 대해 교차 검사
            for (uint32 i = 0; i + 2 < Mesh->Indices.size(); i += 3)
            {
                // 로컬 정점을 월드 좌표로 변환 (행 벡터 규약: V * M)
                const FVector& P0 = Mesh->Vertices[Mesh->Indices[i]].Position;
                const FVector& P1 = Mesh->Vertices[Mesh->Indices[i + 1]].Position;
                const FVector& P2 = Mesh->Vertices[Mesh->Indices[i + 2]].Position;

                FVector W0 = {
                    P0.X * W.M[0][0] + P0.Y * W.M[1][0] + P0.Z * W.M[2][0] + W.M[3][0],
                    P0.X * W.M[0][1] + P0.Y * W.M[1][1] + P0.Z * W.M[2][1] + W.M[3][1],
                    P0.X * W.M[0][2] + P0.Y * W.M[1][2] + P0.Z * W.M[2][2] + W.M[3][2]
                };
                FVector W1 = {
                    P1.X * W.M[0][0] + P1.Y * W.M[1][0] + P1.Z * W.M[2][0] + W.M[3][0],
                    P1.X * W.M[0][1] + P1.Y * W.M[1][1] + P1.Z * W.M[2][1] + W.M[3][1],
                    P1.X * W.M[0][2] + P1.Y * W.M[1][2] + P1.Z * W.M[2][2] + W.M[3][2]
                };
                FVector W2 = {
                    P2.X * W.M[0][0] + P2.Y * W.M[1][0] + P2.Z * W.M[2][0] + W.M[3][0],
                    P2.X * W.M[0][1] + P2.Y * W.M[1][1] + P2.Z * W.M[2][1] + W.M[3][1],
                    P2.X * W.M[0][2] + P2.Y * W.M[1][2] + P2.Z * W.M[2][2] + W.M[3][2]
                };

                float Dist = 0.0f;
                if (RayTriangleIntersect(Ray, W0, W1, W2, Dist))
                {
                    if (Dist < ClosestDist)
                    {
                        ClosestDist = Dist;
                        ClosestActor = Actor;
                    }
                }
            }
        }
    }

    return ClosestActor;
}
