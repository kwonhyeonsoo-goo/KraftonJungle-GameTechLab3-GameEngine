#pragma once
#include "CoreMinimal.h"

class ENGINE_API UObject
{
public: 
	UObject(uint32 InUUID, size_t InObjectType) :UUID(InUUID), ObjectType(InObjectType) {
	};
	virtual ~UObject() = default;
	


	static int32 GetTotalBytes();
	static int32 GetTotalCounts();

private:
	inline static int32 TotalAllocationBytes = 0;
	inline static int32 TotalAllocationCounts = 0;

	size_t ObjectType;
	uint32 UUID;
};

