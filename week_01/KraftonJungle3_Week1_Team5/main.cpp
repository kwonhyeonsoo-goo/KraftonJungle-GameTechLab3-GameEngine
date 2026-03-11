#include <windows.h>

// D3D 사용에 필요한 라이브러리들을 링크합니다.
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D 사용에 필요한 헤더파일들을 포함합니다.
#include <d3d11.h>
#include <d3dcompiler.h>

#include "UGameApp.h"
#include "UWindow.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nShowCmd)
{
	try
	{
		UWindow::FWindowDesc winDesc;
		winDesc.ClassName = L"JungleWindowClass";
		winDesc.Title = L"Game Tech Lab";
		winDesc.Width = 1456;
		winDesc.Height = 1024;
		winDesc.Style = WS_OVERLAPPEDWINDOW;

		UGameApp app(hInstance, winDesc);
		return app.Run(nShowCmd);
	}

	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Fatal Error", MB_ICONERROR | MB_OK);
		return -1;
	}
}
