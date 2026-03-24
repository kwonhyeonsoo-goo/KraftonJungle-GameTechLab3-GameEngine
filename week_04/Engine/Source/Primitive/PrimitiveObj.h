#pragma once

#include "PrimitiveBase.h"

class CPrimitiveObj : public CPrimitiveBase
{
public:
	CPrimitiveObj(const FString& FilePath);

private:
	void LoadObj(const FString& FilePath);
};