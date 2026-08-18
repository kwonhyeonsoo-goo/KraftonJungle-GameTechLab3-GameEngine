#pragma once

#include "PrimitiveBase.h"

class FPrimitiveObj : public CPrimitiveBase
{
public:
	FPrimitiveObj();
	FPrimitiveObj(const FString& FilePath);

private:
	void LoadObj(const FString& FilePath);
};