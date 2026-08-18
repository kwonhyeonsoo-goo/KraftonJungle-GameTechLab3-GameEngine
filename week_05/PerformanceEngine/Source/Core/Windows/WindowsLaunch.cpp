#include "WindowsLaunch.h"

#include "Loop.h"

int FWindowsLaunch::Run(HINSTANCE HInstance, const FLaunchConfig& Config)
{
	if (!InitializeProcess())
	{
		Shutdown();
		return -1;
	}

	FLoop Loop;

	if (!Loop.PreInit(HInstance, Config))
	{
		Loop.Exit();
		Shutdown();
		return -1;
	}

	if (!Loop.Init())
	{
		Loop.Exit();
		Shutdown();
		return -1;
	}

	while (!Loop.IsExitRequested())
	{
		Loop.Tick();
	}

	Loop.Exit();
	Shutdown();

	return 0;
}

bool FWindowsLaunch::InitializeProcess()
{
	ComResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(ComResult) && ComResult != RPC_E_CHANGED_MODE)
	{
		MessageBox(nullptr, L"CoInitializeEx failed", L"COM Error", MB_OK);
		return false;
	}

	return true;
}

void FWindowsLaunch::Shutdown()
{
	if (SUCCEEDED(ComResult) || ComResult == S_FALSE)
	{
		CoUninitialize();
	}

	ComResult = E_FAIL;
}
