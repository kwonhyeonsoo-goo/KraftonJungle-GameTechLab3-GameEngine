#include "FEditorEngine.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	FEditorEngine Engine;
	if (!Engine.Initialize(hInstance))
		return -1;

	Engine.Run();
	//Engine.Shutdown(); // ~FEingine() called shutdown

	return 0;
}
