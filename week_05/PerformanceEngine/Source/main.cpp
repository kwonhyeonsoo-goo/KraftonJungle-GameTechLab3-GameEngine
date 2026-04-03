#include <Windows.h>

#include "Core/Core.h"
#include "Core/Windows/WindowsLaunch.h"

extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0x00000001;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{

	FLaunchConfig Config;
	Config.Title = L"PerformanceEngine";
	Config.Width = 1280;
	Config.Height = 720;

	FWindowsLaunch Launch;
	return Launch.Run(hInstance, Config);
}
