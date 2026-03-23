#pragma once
#include "CoreMinimal.h"
#include "EngineAPI.h"
enum class EEngineShowFlags : uint64
{
	SF_Primitives = 1 << 0,
	SF_BillboardText = 1 << 1,
	// SF_Collision   = 1 << 2,
	 // SF_Grid        = 1 << 3,
	 // SF_Fog         = 1 << 4,
};
class ENGINE_API FShowFlags
{
public:
	FShowFlags()
		: Flags(
			static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
			static_cast<uint64>(EEngineShowFlags::SF_BillboardText)){}
	void SetFlag(EEngineShowFlags InFlag, bool bEnabled);
	bool HasFlag(EEngineShowFlags InFlag)const;
	void ToggleFlag(EEngineShowFlags InFlag);
private:
	uint64 Flags;
};