#include "BoundingBox.h"
#include <algorithm>
#include "Scene/SceneTypes.h"

void FBoundingBox::Encapsulate(const FBoundingBox& Other)
{
	Min.X = std::min(Min.X, Other.Min.X);
	Min.Y = std::min(Min.Y, Other.Min.Y);
	Min.Z = std::min(Min.Z, Other.Min.Z);
	Max.X = std::max(Max.X, Other.Max.X);
	Max.Y = std::max(Max.Y, Other.Max.Y);
	Max.Z = std::max(Max.Z, Other.Max.Z);
}

bool FBoundingBox::IntersectsRay(const FRay& Ray) const
{
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();

    for (int32 i = 0; i < 3; i++)
    {
        float Origin = Ray.Origin[i];
        float Dir = Ray.Direction[i];
        float BMin = Min[i];
        float BMax = Max[i];

        if (std::abs(Dir) < 1e-8f)
        {
            // 레이가 슬랩에 평행
            if (Origin < BMin || Origin > BMax) return false;
        }
        else
        {
            float t1 = (BMin - Origin) / Dir;
            float t2 = (BMax - Origin) / Dir;
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax) return false;
        }
    }
    return true;
}
