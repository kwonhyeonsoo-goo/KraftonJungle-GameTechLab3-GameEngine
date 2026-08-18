#include "BoundingBox.h"
#include <algorithm>
#include "Scene/SceneTypes.h"
#include "Picking/PickingSystem.h"

void FBoundingBox::Encapsulate(const FBoundingBox& Other)
{
	Min.X = std::min(Min.X, Other.Min.X);
	Min.Y = std::min(Min.Y, Other.Min.Y);
	Min.Z = std::min(Min.Z, Other.Min.Z);
	Max.X = std::max(Max.X, Other.Max.X);
	Max.Y = std::max(Max.Y, Other.Max.Y);
	Max.Z = std::max(Max.Z, Other.Max.Z);
}

void FBoundingBox::Encapsulate(const FVector& Other)
{
    if (Other.X  < Min.X ) Min.X  = Other.X;
    if (Other.Y  < Min.Y ) Min.Y  = Other.Y;
    if (Other.Z  < Min.Z ) Min.Z  = Other.Z;
    if (Other.X  > Max.X ) Max.X  = Other.X;
    if (Other.Y  > Max.Y ) Max.Y  = Other.Y;
    if (Other.Z  > Max.Z ) Max.Z  = Other.Z;
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

bool FBoundingBox::Contains(const FVector& Center) const
{
    return (Center.X >= Min.X && Center.X <= Max.X) &&
        (Center.Y >= Min.Y && Center.Y <= Max.Y) &&
        (Center.Z >= Min.Z && Center.Z <= Max.Z);
}
