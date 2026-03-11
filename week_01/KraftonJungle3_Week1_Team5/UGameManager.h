#pragma once

#include "UPrimitive.h"
#include "Macro.h"
#include "Enum.h"

class UGameManager : public UPrimitive
{
	UGameManager();
	~UGameManager() override;

	DECLARE_SINGLETON(UGameManager);
};

