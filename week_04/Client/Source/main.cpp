#include "Core/FEngine.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	FEngine Engine;
	if (!Engine.Initialize(hInstance, L"Jungle Client", 1280, 720))
		return -1;

	Engine.Run();
	Engine.Shutdown();

	return 0;
}
