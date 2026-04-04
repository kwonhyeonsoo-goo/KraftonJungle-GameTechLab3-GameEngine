#include "BoundingBox.h"
#include <algorithm>

void FBoundingBox::Encapsulate(const FBoundingBox& Other)
{
	Min.X = std::min(Min.X, Other.Min.X);
	Min.Y = std::min(Min.Y, Other.Min.Y);
	Min.Z = std::min(Min.Z, Other.Min.Z);
	Max.X = std::max(Max.X, Other.Max.X);
	Max.Y = std::max(Max.Y, Other.Max.Y);
	Max.Z = std::max(Max.Z, Other.Max.Z);
}
