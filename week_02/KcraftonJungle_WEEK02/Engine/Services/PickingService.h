#pragma once
#include "../Foundation/Containers/FVector.h"
#include "../Foundation/Containers/FMatrix.h"
#include "../Foundation/Math/TArray.h"
#include "../ObjectKernel/UObject.h"
#include "../World/UPrimitiveComponent.h"


struct Ray {
    FVector Origin;
    FVector Direction;

    Ray(const FVector& Origin, const FVector& Direction)
        : Origin(Origin) 
        , Direction(Direction)
    {
    }

    Ray() = default;
};

class PickingService
{
public:
    // 스크린 좌표 → 월드 Ray 생성
    static Ray ScreenToRay(int32 screenX, int32 screenY,
        int32 viewportW, int32 viewportH,
        const FMatrix& viewProjInverse);

    // Ray → 교차한 컴포넌트 반환 (없으면 nullptr)
    static UPrimitiveComponent* Pick(const Ray& ray,
        const TArray<UObject*>& objects);

    // 보조: Ray-AABB 교차 테스트
    static bool IntersectsAABB(const Ray& ray,
        const FVector& min,
        const FVector& max,
        float& outT);
};

