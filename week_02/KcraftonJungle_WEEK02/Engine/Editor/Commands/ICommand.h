#pragma once
#include "../../Foundation/Core/CoreTypes.h"

class ICommand
{
public:
	virtual ~ICommand() = default;
	virtual void Execute() = 0;
	virtual void Undo() = 0;
	virtual FString GetDescription() const = 0;
};

