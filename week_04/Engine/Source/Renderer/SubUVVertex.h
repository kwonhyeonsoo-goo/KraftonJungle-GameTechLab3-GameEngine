#pragma once

#include "CoreMinimal.h"

struct ENGINE_API FSubUVVertex
{
	FVector Position;
	FVector2 UV;

	FSubUVVertex()
		: Position(), UV()
	{
	}

	FSubUVVertex(const FVector& InPosition, const FVector2& InUV)
		: Position(InPosition), UV(InUV)
	{
	}
};