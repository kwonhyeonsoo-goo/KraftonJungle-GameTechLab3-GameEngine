#pragma once

#include "PrimitiveBase.h"

class ENGINE_API FPrimitiveObj : public CPrimitiveBase
{
public:
	enum class EImportAxisMode : uint8
	{
		ZUp,
		YUpToZUp
	};

	FPrimitiveObj();
	FPrimitiveObj(const FString& FilePath);

	static void SetImportAxisMode(EImportAxisMode InMode);
	static EImportAxisMode GetImportAxisMode();

private:
	void LoadObj(const FString& FilePath);

	static EImportAxisMode ImportAxisMode;
};
