#include <Windows.h>

#include "Core/Core.h"
#include "Core/Windows/WindowsLaunch.h"

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{

	FLaunchConfig Config;
	Config.Title = L"PerformanceEngine";
	Config.Width = 1280;
	Config.Height = 720;

	FWindowsLaunch Launch;
	return Launch.Run(hInstance, Config);
}
