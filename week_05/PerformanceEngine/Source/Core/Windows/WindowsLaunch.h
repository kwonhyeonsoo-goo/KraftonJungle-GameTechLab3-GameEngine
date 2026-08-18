#pragma once
#include <Windows.h>

#include "Types/PlatformTypes.h"

struct FLaunchConfig
{
	const wchar_t* Title = L"PerformanceEngine";
	int32 Width = 1280;
	int32 Height = 720;
};

class FWindowsLaunch
{
public:
	int Run(HINSTANCE HInstance, const FLaunchConfig& Config);

private:
	bool InitializeProcess();
	void Shutdown();

private:
	HRESULT ComResult = E_FAIL;
};

