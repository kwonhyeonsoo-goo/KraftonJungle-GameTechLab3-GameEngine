#include "Rect.h"

bool FRect::ContainsPoint(const FPoint& Point) const
{
	return Point.X >= Position.X && Point.X <= Position.X + Size.X &&
		Point.Y >= Position.Y && Point.Y <= Position.Y + Size.Y;
}
