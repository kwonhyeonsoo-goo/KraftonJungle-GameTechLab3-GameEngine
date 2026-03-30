#pragma once
#include "CoreMinimal.h"
#include <windows.h>
#include "Core/Paths.h"

enum class ENGINE_API EFileDialogType : uint8
{
	Open,
	Save
};

std::string ENGINE_API GetJsonFilePath(EFileDialogType Type);
std::string ENGINE_API GetObjFilePath(EFileDialogType Type);
